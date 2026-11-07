#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "IUtils.h"
#include "Events/System/timerEvent.h"
#include "REF.h"
#include "IUtils.h"
#include "commonError.h"
#include "SERVICE_id.h"
#include "IInstance.h"
#include "broadcaster.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <exception>
#include <string>
#include <time.h>
#include "grainReaderService.h"
#include "event_mt.h"
#include "listenerBuffered1Thread.h"
#include "ioBuffer.h"
#include "msg.h"
#include "tools_mt.h"
#include "events_grainReaderService.hpp"
#include "unknown.h"
#include "msgFactory.h"
#include "md/md_GetUserNonceREQ.h"
#include "md/md_GetUserNonceRSP.h"
#include "CDatabaseRocksdb.h"
#include "md/md_GetNodeInfoREQ.h"
#include "md/md_GetNodeInfoRSP.h"
#include "md/md_GetContractDataREQ.h"
#include "md/md_GetContractDataRSP.h"
#include "md/md_GetValuesREQ.h"
#include "md/md_GetValuesRSP.h"
#include "md/md_GetContractREQ.h"
#include "md/md_GetContractRSP.h"
#include "md/md_GetStateREQ.h"
#include "md/md_GetStateRSP.h"
#include "md/md_GetAddressInfoREQ.h"
#include "md/md_GetAddressInfoRSP.h"
#include "md/md_CallViewREQ.h"
#include "md/md_CallViewRSP.h"
#include "md/md_GetGranuleREQ.h"
#include "md/md_GetGranuleRSP.h"
#include "cells.h"
#include "NodeGranuleIO.h"
#include "CallStack.h"
#include "convert.h"
#include "CDatabaseLiteNode.h"
#include "httpConnection.h"

bool GrainReader::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;

    if (mode_ == Lite) {
        if (uplink_http_addr_.empty())
            return true;
        db_state_3 = new CDatabaseLiteNode(db_path_, uplink_http_addr_);
        auto prev_root = load_last_block(db_state_3.get());
        root = getRoot(db_state_3.get(), prev_root);
        
        contractHost = new ContractHost();
        contractHost->granules = new NodeGranuleIO(db_state_3);
        contractHost->source = new NodeContractSourceIO(db_state_3);
        logErr2("GrainReader: lite mode, db=%s, http=%s", db_path_.c_str(), uplink_http_addr_.c_str());
    } else {
        logErr2("GrainReader: full mode, wait ServiceInit");
        SECURE sec;
        sec.use_ssl = false;
        for (auto& z : http_granule_addr_)
            sendEvent(ServiceEnum::HTTP, new httpEvent::DoListen(z, sec, this));
        sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(GrainReader::TIMER_STALE_CLEANUP, NULL, NULL, 60., this));
    }
    return true;
}
bool GrainReader::Service::on_StaleGranules(const bcEvent::StaleGranules* e)
{
    MUTEX_INSPECTOR;
    int64_t now = (int64_t)time(NULL);
    for (auto& z : e->v)
        stale_cache[z.first] = { z.second, now };
    return true;
}
void GrainReader::Service::cleanup_stale(int64_t now)
{
    for (auto it = stale_cache.begin(); it != stale_cache.end(); ) {
        if (it->second.ts + STALE_TTL_SEC < now) it = stale_cache.erase(it);
        else ++it;
    }
}
bool GrainReader::Service::get_stale(const std::string& hash, std::string& out)
{
    auto it = stale_cache.find(hash);
    if (it == stale_cache.end()) return false;
    out = it->second.body;
    return true;
}
bool GrainReader::Service::RequestIncoming(const httpEvent::RequestIncoming* e)
{
    MUTEX_INSPECTOR;
    if (mode_ != Full) return true;
    auto uri = std::string(e->req->url);
    if (uri.rfind("/granule", 0) != 0) return true;

    std::map<std::string,std::string> q;
    auto pos = uri.find('?');
    if (pos != std::string::npos) {
        std::string qs = uri.substr(pos + 1);
        size_t start = 0;
        while (start < qs.size()) {
            size_t amp = qs.find('&', start);
            std::string pair = qs.substr(start, amp == std::string::npos ? std::string::npos : amp - start);
            size_t eq = pair.find('=');
            if (eq != std::string::npos) q[pair.substr(0, eq)] = pair.substr(eq + 1);
            if (amp == std::string::npos) break;
            start = amp + 1;
        }
    }
    std::string addr_hex = q.count("addr") ? q["addr"] : "";
    std::string hash_hex = q.count("hash") ? q["hash"] : "";

    HTTP::Response rsp(e->req);
    rsp.setHeader("Content-Type", "application/json");
    if (addr_hex.empty()) {
        rsp.setStatus(400); rsp.make_response(R"({"error":"addr required"})"); return true;
    }
    std::string addr_bin = base16::decode(addr_hex);
    if (addr_bin.size() != 32) {
        rsp.setStatus(400); rsp.make_response(R"({"error":"addr must be 32 bytes hex"})"); return true;
    }
    std::string expected_hash = hash_hex.empty() ? "" : base16::decode(hash_hex);
    if (!expected_hash.empty() && expected_hash.size() != 32) {
        rsp.setStatus(400); rsp.make_response(R"({"error":"hash must be 32 bytes hex"})"); return true;
    }

    // 1) актуальное
    std::string body;
    int r = db_state_3->getGranule(addr_bin, &body);
    if (r == 0 && !body.empty()) {
        if (expected_hash.empty() || blake2b_hash(body).container == expected_hash) {
            rsp.make_response(R"({"body":")" + base16::encode(body) + R"(","stale":false})");
            return true;
        }
    }
    // 2) stale_cache по hash
    if (!expected_hash.empty()) {
        std::string stale_body;
        if (get_stale(expected_hash, stale_body)) {
            rsp.make_response(R"({"body":")" + base16::encode(stale_body) + R"(","stale":true})");
            return true;
        }
    }
    // 3) 404
    rsp.setStatus(404);
    rsp.make_response(R"({"error":"not found"})");
    return true;
}
bool GrainReader::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    if (e->tid == GrainReader::TIMER_STALE_CLEANUP) {
        cleanup_stale((int64_t)time(NULL));
        return true;
    }
    return true;
}
bool GrainReader::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;
    return false;
}

bool GrainReader::Service::handleEvent(const REF_getter<Event::Base> &e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try
    {
        MUTEX_INSPECTOR;
        auto &ID = e->id;
        switch (ID)
        {
        case bcEventEnum::LiteSetUplinkREQ:
            return on_LiteSetUplinkREQ((const bcEvent::LiteSetUplinkREQ*)e.get());
        case bcEventEnum::StaleGranules:
            return on_StaleGranules((const bcEvent::StaleGranules *)e.get());
        case httpEventEnum::RequestIncoming:
            return RequestIncoming((const httpEvent::RequestIncoming *)e.get());            
        case bcEventEnum::ClientMsg:
            return ClientMsg((const bcEvent::ClientMsg *)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit *)e.get());
        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot *)e.get());
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer *)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm *)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService *)e.get());
        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor *ev = static_cast<const rpcEvent::IncomingOnAcceptor *>(e.get());
            auto &IDA = ev->e->id;

            switch (IDA)
            {
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg *)ev->e.get());
            default:
                throw CommonError("unhabdled ev %d %s", IDA, iUtils->genum_name(IDA));
            }
        }
        break;
        case rpcEventEnum::IncomingOnConnector:
        {
            const rpcEvent::IncomingOnConnector *ev = static_cast<const rpcEvent::IncomingOnConnector *>(e.get());
            auto &IDC = ev->e->id;
            switch (IDC)
            {
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg *)ev->e.get());

            default:
                throw CommonError("unhabdled ev %d %s", IDC, iUtils->genum_name(IDC));
            }

        }
        break;
        default:
            throw CommonError("unhabdled ev %d %s", ID, iUtils->genum_name(ID));
        }
    }
    catch (std::exception &e)
    {
        logErr2("GrainReader std::exception  %s", e.what());
    }
    XPASS;
    return false;
}


GrainReader::Service::~Service()
{
}

GrainReader::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins, Mode mode)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins),
      mode_(mode)
{
    if (mode_ == Lite) {
        uplink_http_addr_ = ins->getConfig()->get_string("node_http_addr", "", "full node /granule addr (required for lite)");
        db_path_        = ins->getConfig()->get_string("grainreader_db_path", "db/gr0", "lite RocksDB path");
    } else {
        http_granule_addr_ = ins->getConfig()->get_tcpaddr("http_granule_addr", "127.0.0.1:3301", "HTTP /granule addr (full mode)");
    }
}

bool GrainReader::Service::ServiceInit(const bcEvent::ServiceInit *e)
{

    if (mode_ == Lite) return true;   // lite: on_startService уже создал всё
    conf = e;
    db_state_3 = e->db;
    auto prev_root=load_last_block(db_state_3.get());
    root=getRoot(db_state_3.get(),prev_root);
    contractHost = new ContractHost();
    contractHost->granules = new NodeGranuleIO(db_state_3);
    contractHost->source = new NodeContractSourceIO(db_state_3);
    return true;
}
bool GrainReader::Service::InvalidateRoot(const bcEvent::InvalidateRoot *e)
{
    auto prev_root=load_last_block(db_state_3.get());
    root=getRoot(db_state_3.get(),prev_root);
    return true;
}

bool GrainReader::Service::ClientMsg(const bcEvent::ClientMsg *e)
{

    MUTEX_INSPECTOR;
    inBuffer in(e->msg);

    auto p = in.get_PN();
    REF_getter<MsgData::Base> b=msgFactory.create(p);
    b->unpack(in);
    auto hash=b->getHash();

    switch(p)
    {
    case msgid::GetGranuleREQ:
    {
        auto pp = (MsgData::GetGranuleREQ*)b.get();
        REF_getter<MsgData::GetGranuleRSP> rsp = new MsgData::GetGranuleRSP;

        if (pp->addr.size() != 32) {
            logErr2("GetGranuleREQ: bad addr (size %zu)", pp->addr.size());
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        if (!pp->hash.empty() && pp->hash.size() != 32) {
            logErr2("GetGranuleREQ: bad hash (size %zu)", pp->hash.size());
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }

        // 1) актуальное
        std::string body;
        int r = db_state_3->getGranule(pp->addr, &body);
        if (r == 0 && !body.empty()) {
            if (pp->hash.empty() || blake2b_hash(body).container == pp->hash) {
                rsp->found = true;
                rsp->stale = false;
                rsp->body = body;
            }
        }

        // 2) stale_cache по hash (только full)
        if (!rsp->found && !pp->hash.empty() && mode_ == Full) {
            std::string stale_body;
            if (get_stale(pp->hash, stale_body)) {
                rsp->found = true;
                rsp->stale = true;
                rsp->body = stale_body;
            }
        }

        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }        
    case msgid::GetAddressInfoREQ: {
        auto pp = (MsgData::GetAddressInfoREQ*)b.get();
        REF_getter<MsgData::GetAddressInfoRSP> rsp = new MsgData::GetAddressInfoRSP;
        // rsp->addr = pp->addr;
        auto u = cells::getAddressStateNoCreate(root.get(), db_state_3.get(),pp->addr);   // normalize — в getPath
        if (u.valid()) {
            rsp->found = true;
            rsp->balance = u->balance;
            rsp->nonce = u->nonce;
            // rsp->name = pp->addr.addr;
            for (auto& [id, k] : u->keys) {
                MsgData::KeyInfo ki;
                ki.id = id;
                ki.type = k.type;
                ki.keybuffer = base16::encode(k.keybuffer);
                rsp->keys.push_back(ki);
            }
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    case msgid::GetUserNonceREQ:
    {
        auto pp=(MsgData::GetUserNonceREQ*) b.get();
        // ▼▼▼ NoCreate — не создаём адрес (спам RPC)
        auto u = cells::getAddressStateNoCreate(root.get(),db_state_3.get(), pp->user_address);
        logErr2("case msgid::GetUserNonceREQ: pp->user_address %s",pp->user_address.str().c_str());
        REF_getter<MsgData::GetUserNonceRSP> rsp = new MsgData::GetUserNonceRSP;
        if (u.valid()) {
        logErr2("case msgid::GetUserNonceREQ: pp->user_address %s found",pp->user_address.str().c_str());
            
            rsp->nonce = u->nonce;
            rsp->balance = u->balance;
            rsp->found = true;
        } else {
        logErr2("case msgid::GetUserNonceREQ: pp->user_address %s !found",pp->user_address.str().c_str());
            rsp->nonce = 0;
            rsp->balance = 0;
            rsp->found = false;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        // ▲▲▲
    }
    break;
    case msgid::GetNodeInfoREQ:
    {
        auto pp=(MsgData::GetNodeInfoREQ*) b.get();
        REF_getter<MsgData::GetNodeInfoRSP> rsp = new MsgData::GetNodeInfoRSP;
        auto n = cells::getNodeNoCreate(root.get(),db_state_3.get(), pp->node_name);
        if (n.valid()) {
            rsp->found = true;
            rsp->name = n->getName();
            rsp->owner = n->get_owner();
            rsp->ed_pk = base16::encode(n->get_ed_pk());
            rsp->bls_pk = base16::encode(n->get_bls_pk().serialize());
            rsp->ip = n->get_ip();
            rsp->enabled = n->isEnabled();
            rsp->missed_rounds = n->missed_rounds;
            rsp->answered_rounds = n->answered_rounds;
            rsp->full_stake = n->get_full_stake();
            rsp->stakes = n->getStakes();
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    case msgid::GetValuesREQ:
    {
        REF_getter<MsgData::GetValuesRSP> rsp = new MsgData::GetValuesRSP;
        auto v = cells::getValuesNoCreate(root.get(),db_state_3.get());
        if (v.valid()) {
            rsp->found = true;
            rsp->fees = v->values;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    case msgid::GetContractDataREQ:
    {
        auto pp=(MsgData::GetContractDataREQ*) b.get();
        REF_getter<MsgData::GetContractDataRSP> rsp = new MsgData::GetContractDataRSP;
        rsp->contract = pp->contract.container;
        rsp->key = pp->key.container;
        auto d = cells::getContractDataNoCreate(root.get(),db_state_3.get(), pp->contract, pp->key);
        if (d.valid()) {
            rsp->found = true;
            rsp->value = d->container;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    case msgid::GetContractREQ:
    {
        auto pp=(MsgData::GetContractREQ*) b.get();
        REF_getter<MsgData::GetContractRSP> rsp = new MsgData::GetContractRSP;
        rsp->contract = pp->contract;
        auto c = cells::getContractNoCreateConst(root.get(), db_state_3.get(), pp->contract);
        if (c.valid()) {
            rsp->found = true;
            rsp->owners = c->owners;
            rsp->src = c->src;
            auto act = cells::getContractActivityNoCreateConst(root.get(), db_state_3.get(), pp->contract);
            if(!act.valid())
                throw CommonError("if(!act.valid())");
            rsp->ttl = act->ttl;   // ← activity.ttl (маркер), не bc_contract.ttl (0)
            rsp->update_time = act->__update_time;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }

// в ClientMsg switch(p):
    case msgid::GetStateREQ:
    {
        REF_getter<MsgData::GetStateRSP> rsp = new MsgData::GetStateRSP;
        auto st = cells::getStateNoCreateConst(root.get(), db_state_3.get());
        if (st.valid()) {
            rsp->found = true;
            rsp->total_supply = st->total_supply.str();
            rsp->total_stake  = st->total_stake.str();
            rsp->account_count = st->account_count;
            rsp->node_count = st->node_count;
            rsp->contract_count = st->contract_count;
            rsp->tx_count = st->tx_count;
            rsp->block_height = st->block_height;
            rsp->genesis_time = st->genesis_time;
            rsp->chain_id = st->chain_id;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    case msgid::CallViewREQ:
    {
        auto pp = (MsgData::CallViewREQ*)b.get();
        REF_getter<MsgData::CallViewRSP> rsp = new MsgData::CallViewRSP;

        auto cc = contractHost->get_contract(root.get(), pp->contract);
        if (!cc.valid()) {
            rsp->found = false;
            rsp->error = true;
            rsp->result_json = "contract not found: " + pp->contract.container;
        } else if (cc->views.find(pp->view) == cc->views.end()) {
            rsp->found = false;
            rsp->error = true;
            rsp->result_json = "view not found: " + pp->view;
        } else {
            rsp->found = true;
            nlohmann::json params;
            bool params_ok = true;
            try {
                params = pp->params_json.empty()
                       ? nlohmann::json::object()
                       : nlohmann::json::parse(pp->params_json);
            } catch (const std::exception& e) {
                params_ok = false;
                rsp->error = true;
                rsp->result_json = std::string("params: invalid json: ") + e.what();
            }
            if (params_ok) {
                REF_getter<CallStack> cs = new CallStack;
                CONTRACT_id caller;           // внешний вызов → пустой caller
                JSIValue r = cc->call_contract_view(pp->view, params, cs, contractHost, pp->timeout_sec);
                if (JSI_IsException(r)) {
                    std::string emsg;
                    JSIValue exc = JSI_GetException(cc->ctx);
                    if (!JSI_IsUndefined(exc)) {
                        const char* s = JSI_ToCString(cc->ctx, exc);
                        if (s) { emsg = s; JSI_FreeCString(cc->ctx, s); }
                        JSI_FreeValue(cc->ctx, exc);
                    }
                    JSI_FreeValue(cc->ctx, r);
                    rsp->error = true;
                    rsp->result_json = emsg.empty() ? "view exception" : emsg;
                } else {
                    rsp->result_json = serialize_value(cc->ctx, r);
                    JSI_FreeValue(cc->ctx, r);
                }
            }
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    default:
        logErr2("unhanddledf msg %s",msgName(p));
    }

    return true;
}

void registerGrainReaderService(const char *pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if (pn)
    {
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::GrainReader, "GrainReader", getEvents_grainReaderService());
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::GrainReaderLite, "GrainReaderLite", getEvents_grainReaderService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::GrainReader,     GrainReader::Service::constructFull, "GrainReader");
        iUtils->registerService(ServiceEnum::GrainReaderLite, GrainReader::Service::constructLite, "GrainReaderLite");
        regEvents_grainReaderService();
    }

    XPASS;
}
bool GrainReader::Service::on_LiteSetUplinkREQ(const bcEvent::LiteSetUplinkREQ* e)
{
    MUTEX_INSPECTOR;
    if (mode_ != Lite) return true;
    std::string db_path = "db/gr0";
    db_state_3 = new CDatabaseLiteNode(db_path, e->addr);
    auto prev_root = load_last_block(db_state_3.get());
    root = getRoot(db_state_3.get(), prev_root);
    contractHost = new ContractHost();
    contractHost->granules = new NodeGranuleIO(db_state_3);
    contractHost->source = new NodeContractSourceIO(db_state_3);
    logErr2("GrainReader lite: node=%s", e->addr.c_str());
    return true;
}