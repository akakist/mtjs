#include "Events/System/Net/httpEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "base16.h"
#include "commonError.h"
#include "Events/Tools/telnetEvent.h"

#include "blake2bHasher.h"
#include "REF.h"
#include "Events/Tools/webHandlerEvent.h"
#include "IUtils.h"
#include "SERVICE_id.h"
#include "IInstance.h"
#include "broadcaster.h"
#include "THASH_id.h"
#include "QUORUM.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <exception>
#include <string>
#include <optional>
#include <cstdarg>
#include <cstdio>
#include <time.h>
#include <map>
#include <vector>
#include "token2gas.h"
#include "nodeService.h"
#include "epoll_socket_info.h"
#include "event_mt.h"
#include "events_nodeService.hpp"
#include "getenv2.h"
#include "httpConnection.h"
#include "listenerBase.h"
#include "msg.h"
#include "ioBuffer.h"
#include "s_ed.h"
#include "unknown.h"
#include "listenerBuffered1Thread.h"
#include "t_params.h"
#include "__crc32.h"
#include "nodeService.h"
#include "CDatabaseRocksdb.h"
#include "NodeMsgSource.h"
#include "genesis.h"
#include "proceed_merkle_on_transaction_pool_hashers.h"


#include "yyjson_to_quickjs.h"
#include "js_tools.h"
#include "jsi/api/ContractHost.h"
#include "NodeGranuleIO.h"
#include "CallStack.h"
#include "CallContext.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/snapshot.h>
#include <rocksdb/utilities/checkpoint.h>
#include "s_key.h"
#include "cells.h"



static REF_getter<ContractHost> make_node_contract_host(const REF_getter<IDatabase>& db) {
    REF_getter<ContractHost> h = new ContractHost();
    h->granules = new NodeGranuleIO(db);
    h->source = new NodeContractSourceIO(db);
    return h;
}
// ============================================================
// dump helpers (для /dump)
// ============================================================
static nlohmann::json dump_hb(const REF_getter<MsgData::HeartBeatREQ>& hb)
{
    if (!hb.valid()) return nullptr;
    return {
        {"prev",   hb->prev_root_hash_1.str()},
        {"epoch",  hb->new_epoch},
        {"leader", hb->node_leader.container},
        {"hash",   hb->getHash().str()}
    };
}

static nlohmann::json dump_leader_cert(const REF_getter<MsgData::LeaderCert>& lc)
{
    if (!lc.valid()) return nullptr;
    nlohmann::json j;
    j["hb"] = dump_hb(lc->hb);
    for (auto& z : lc->node_validators) j["validators"].push_back(z.container);
    j["agg_sig"] = base16::encode(lc->agg_sig.serialize());
    return j;
}

static nlohmann::json dump_block_info(const REF_getter<MsgData::BlockInfo>& bi)
{
    if (!bi.valid()) return nullptr;
    return {
        {"root",       bi->new_root_hash1.str()},
        {"tx_hash",    bi->tx_hash.str()},
        {"diffs_hash", bi->diffs_hash.str()},
        {"att_hash",   bi->att_data_hash.str()},
        {"hb",         dump_hb(bi->heart_beat)}
    };
}

static nlohmann::json dump_block_accepted2(const REF_getter<MsgData::BlockAccepted2REQ>& ba2)
{
    if (!ba2.valid()) return nullptr;
    nlohmann::json j;
    if (ba2->blockAcceptedREQ.valid()) {
        j["blockInfo"] = dump_block_info(ba2->blockAcceptedREQ->blockInfo);
        for (auto& z : ba2->blockAcceptedREQ->node_validators) j["node_validators"].push_back(z.container);
        j["agg_sig"] = base16::encode(ba2->blockAcceptedREQ->agg_sig.serialize());
    }
    for (auto& z : ba2->node_diff_validators) j["node_diff_validators"].push_back(z.container);
    j["agg_diff_sig"] = base16::encode(ba2->agg_diff_sig.serialize());
    return j;
}

void Node::Service::dump(nlohmann::json& j)
{
    j["node"] = this_node_name.container;
    j["prev_root_hash"] = prev_root_hash_Z().str();
    j["epoch"] = epoch_current();
    j["sync_in_progress"] = (bool)sync_in_progress;


    // l_blocks (все события)
    for (auto& [prev, bt] : l_blocks) {
        auto& lb = j["l_blocks"][base16::encode(prev.container)];
        auto& li = bt;


        // validate_block rsp
        for (auto& [bh, v] : bt.ValidateBlockRSP_m) {
            for (auto& rsp : v) {
                if (!rsp.valid()) continue;
                lb["validate_rsp"].push_back({
                    {"block_hash", bh.str()},
                    {"validator",  rsp->node_validator.container},
                    {"blockInfo",  dump_block_info(rsp->blockInfo)},
                    {"sig",        base16::encode(rsp->sig.serialize())},
                    {"tx_count",   (int)rsp->tx_hashes.size()},
                    {"diffs_count",(int)rsp->diffs.size()}
                });
            }
        }

    }

    // tx_pool
    j["tx_pool"]["size"] = (int)transaction_pool_of_leader.size();
    for (auto& [h, tx] : transaction_pool_of_leader) {
        if (!tx.valid()) continue;
        auto sk=tx->getSenderKey();
        j["tx_pool"]["txs"].push_back({
            {"hash", base16::encode(h.container)},
            {"body", tx->tx_body},
            {"sender", sk ? sk->first : std::string()},
            {"key_id", sk ? sk->second : -1}
        });
    }

    // prev_block
    j["prev_block"] = dump_block_accepted2(prev_block);

    // nodes
    // if (db_state.valid()) {
    //     auto an = cells::getAllNodes(root.get(), db_state.get());
    //     for (auto& n : an) {
    //         if (!n.valid()) continue;
    //         j["nodes"].push_back({
    //             {"name", n->getName().container},
    //             {"enabled", n->isEnabled()},
    //             {"stake", std::to_string(n->get_full_stake())},
    //             {"owner", base16::encode(n->get_owner().addr)},
    //             {"ip", n->get_ip()},
    //             {"missed_rounds", n->missed_rounds}
    //         });
    //     }
    // }
}
#include "CDatabaseLiteNode.h"
bool Node::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;


    if (mode_ == Lite) {
        if (uplink_http_addr_.empty())
            return true;
        db_state = new CDatabaseLiteNode(db_name, uplink_http_addr_);
        DBG(logErr2("Node: lite mode, db=%s, http=%s", db_name.c_str(), uplink_http_addr_.c_str()));
    } else {
        DBG(logErr2("Node: full mode, wait ServiceInit"));
        db_state = new CDatabaseRocksdb(db_name);
        SECURE sec;
        sec.use_ssl = false;
        for (auto& z : http_granule_addr_)
            sendEvent(ServiceEnum::HTTP, new httpEvent::DoListen(z, sec, this));
        sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(TIMER_STALE_CLEANUP, NULL, NULL, 60., this));
        prev_block=load_last_block(db_state.get());
        root=getRoot(db_state.get(),prev_block);
        sendEvent(ServiceEnum::Scan, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_state, this));
    }

    
    contractHost=make_node_contract_host(db_state);


    init_genesis(root.get(), db_state.get());

    if(prev_block.valid())
    {
        if(!verify_block_all(prev_block))
            prev_block=NULL;
    }

    for (auto &z : rpc_addr)
    {
        SECURE sec;
        sec.use_ssl = false;
        sendEvent(ServiceEnum::RPC, new rpcEvent::DoListen(z, sec));
    }
    sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(timers::TIMER_PERIODIC_CLOCK, NULL, NULL, 0.05, this));
    sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(timers::TIMER_REPORT_MEM, NULL, NULL, 30., this));

    std::string res;

    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^ds$", "show current element dump", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^go\\s+(.+)$", "go to child element", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^back$", "go to parent", ListenerBase::serviceId));

    // broadcast_MsgEvent_via_broadcaster(e.get(),getMetaFull()->tree_all_nodes);

    return true;
}
#include "tools_mt.h"
bool Node::Service::AddTxREQ(const bcEvent::AddTxREQ *e)
{
    MUTEX_INSPECTOR;
    std::optional<std::string> err;
    auto hash = e->tx->getHash();
    auto xroot=root;
    yyjson_doc* doc = yyjson_read(e->tx->tx_body.data(), e->tx->tx_body.size(), 0);
    if (!doc) { err = "tx_body: invalid json"; }
    else {
        yyjson_val *root = yyjson_doc_get_root(doc);
        yyjson_val *sv = root ? yyjson_obj_get(root, "sender") : nullptr;
        yyjson_val *ki = root ? yyjson_obj_get(root, "key_id") : nullptr;
        std::string sender_hex;
        if (sv && yyjson_is_str(sv)) sender_hex = yyjson_get_str(sv);
        int64_t key_id = -1;
        if (ki && yyjson_is_num(ki)) key_id = yyjson_get_sint(ki);

        if (!sender_hex.empty()) {
            ADDRESS_id senderAddress;
            senderAddress.addr = base16::decode(sender_hex);
            auto u = cells::getAddressStateNoCreateConst(xroot.get(),db_state.get(), senderAddress);
            if (!u.valid()) { err = "sender address not found"; }
            else if (key_id < 0) { err = "key_id required"; }
            else {
                auto k = u->getKey((uint64_t)key_id);
                if (!k) { err = "key_id not found"; }
                else if (!verify(*k, e->tx->sig_ed_bin, hash.container)) { err = "verify failed"; }
                else {
                    e->tx->sender_addr = senderAddress;
                    e->tx->sender_key_id = (uint64_t)key_id;
                    e->tx->sender_resolved = true;
                }
            }
        } else {
            err = "sender required";
        }
        yyjson_doc_free(doc);
    }

    if (!err)
    {
        // бывшая логика PutTxREQ: добавить в локальный пул
        transaction_pool_of_leader.insert_or_assign(hash, e->tx);
        // прямой push на лидера — ОТЛОЖЕН (нужен разбор race с ротацией лидера)
    }
    passEvent(new bcEvent::AddTxRSP(hash, err.has_value(), err ? *err : "transaction added to pool",
                                    poppedFrontRoute(e->route)));
    return true;
}

std::vector<REF_getter<MsgData::TX>> Node::Service::collectTx()
{
    MUTEX_INSPECTOR;
    std::map<std::string, std::map<int64_t, std::vector<REF_getter<MsgData::TX>>>> ordered;
    for (auto &z : transaction_pool_of_leader) {
        auto sk = z.second->getSenderKey();
        if (!sk) {
            logNode("collectTx: skip tx %s (no sender)",
                    base16::encode(z.first.container).c_str());
            continue;
        }
        int64_t nonce = 0;
        auto err = z.second->getNonce(nonce);
        if (err) {
            logNode("collectTransactions: skip tx %s (getNonce: %s)",
                    base16::encode(z.first.container).c_str(), err->c_str());
            continue;
        }
        ordered[sk->first][nonce].push_back(z.second);
    }
    transaction_pool_of_leader.clear();
    std::vector<REF_getter<MsgData::TX>> out;
    for (auto &x : ordered) {
        for (auto &y : x.second) {
            for (auto &z : y.second) {
                out.push_back(z);
                transaction_pool_of_leader.insert_or_assign(z->getHash(), z);
            }
        }
    }
    logNode("collectTransactions: collected %zu (from %zu)",
            out.size(), transaction_pool_of_leader.size());
    return out;
}

static TreeNode filter_tree(const std::vector<NodeElement>& vne, const std::set<NODE_id>& alive)
{
    std::vector<NodeElement> filtered;
    for (auto& n : vne)
        if (alive.count(n.name))
            filtered.push_back(n);
    if (filtered.empty())
        filtered = vne;   // все мертвы — buildTree требует непустой
    return buildTree(filtered);
}

void Node::Service::do_start_block(MsgData::HeartBeatREQ* hb)
{
    MUTEX_INSPECTOR;

    // Компакция исполняется в рамках блока: удаления, собранные в этом блоке,
    // применяются после успешной записи. Новый блок — список сбрасывается.
    local_deletes.clear();

    if (!db_state.valid()) {
        logNode("do_start_block: db_state invalid");
        return;
    }
    if (sync_in_progress) {
        logNode("do_start_block: sync in progress");
        return;
    }

    const auto prev = prev_root_hash_Z();


    auto mf = getMetaFull();
    if (!mf.valid()) {
        logNode("do_start_block: meta invalid");
        return;
    }

    REF_getter<MsgData::ValidateBlockREQ> vb = new MsgData::ValidateBlockREQ();
    vb->heart_beat = hb;
    vb->block_timestamp=iUtils->getNow();
    // vb->lc = makeLeaderCert(hb, mf.get());

    auto &bt = l_blocks[prev];

    // ▼▼▼ collectTransactions возвращает отсортированный vector
    auto txs = collectTx();
    for (auto &tx : txs) {
        vb->transaction_bodies.push_back(tx);
    }
    logNode("do_start_block: broadcast ValidateBlockREQ tx_count=%zu (sorted)",
            vb->transaction_bodies.size());
    // ▲▲▲
    if(vb->transaction_bodies.empty())
    {
        // return;
    }
    TreeNode comm_tree = filter_tree(mf->committee_vne, bt.alive);
    broadcast_MsgEvent_via_broadcaster(vb.get(), comm_tree);
}

bool Node::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    if (e->tid == TIMER_STALE_CLEANUP) {
        MUTEX_INSPECTOR;
        cleanup_stale(iUtils->getNow().v);
        return true;
    }
    if(e->tid == TIMER_PERIODIC_CLOCK)
    {
        MUTEX_INSPECTOR;
        if(sync_in_progress)
        {
            logNode("skipped: sync in progress %s %d",__FILE__,__LINE__);
            return true;

        }
        // auto me=cells::getNodeNoCreateConst(root.get(),db_state.get(),this_node_name);
        auto cl=current_leader();
        if(cl.valid() && cl->getName()==this_node_name)
        {
            sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(TIMER_GetTxFromLeader,NULL,NULL,0.02,this));

        }
        return true;
    }
    if(e->tid==timers::TIMER_REPORT_MEM)
    {
        // report_mem();
    }
    return true;
}
bool Node::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;

    switch (e->tid)
    {
    case timers::ALARM_SYNC_WATCHDOG:
    {
        if(!sync_in_progress)
            return true;
        logNode("sync watchdog: no activity for %.1f s — aborting sync, root from DB",SYNC_WATCHDOG_SEC);
        sync_in_progress=false;
        root=getRoot(db_state.get(),prev_block);
        return true;
    }
    case timers::TIMER_BLOCK_EXPIRED:
    {
        logNode("fail leader %s",e->data->container.c_str());
        auto & cli=cli_leader_info[prev_root_hash_Z()];
        if(cli.node_leader.valid() && cli.node_leader->node_leader.container==e->data->container)
            cli.node_leader=nullptr;
        return true;
    }
    case timers::TIMER_GetTxFromLeader:
    {
        auto cl=current_leader();
        if(cl.valid() && cl->getName()==this_node_name)
        {
            auto &bt = l_blocks[prev_root_hash_Z()];
            REF_getter<MsgData::HeartBeatREQ> h=new MsgData::HeartBeatREQ(prev_root_hash_Z(),
                        epoch_current(),
                        this_node_name, iUtils->getNow().v, 0);
            auto mf=getMetaFull();

            // пинг один раз за раунд — дальше цепочку ведут PingRSP -> TIMER_PingRSP
            if (!bt.ping_sent)
            {
                bt.ping_sent = true;
                REF_getter<MsgData::PingREQ> ping = new MsgData::PingREQ();
                ping->hb = h;
                broadcast_MsgEvent_via_broadcaster(ping.get(), mf->tree_all_nodes);
            }
        }
        return true;
    }
    case timers::TIMER_PingRSP:
    {
        if(sync_in_progress)
        {
            return true;
        }
        auto &bt = l_blocks[prev_root_hash_Z()];
        THASH_id hb_hash;
        hb_hash.container=e->data->container;
        MsgData::HeartBeatREQ* hb=dynamic_cast<MsgData::HeartBeatREQ*> (e->cookie.get());
        if(!hb)
        throw CommonError("if(!hb)");

        // пинг финиширован: финализируем живой сет и переходим к GetTx
        bt.alive.clear();
        for (auto& z : bt.PingRSP_m[hb_hash])
            bt.alive.insert(z.first);
        bt.ping_done = true;
        logNode("ping done: alive=%zu", bt.alive.size());

        // стартуем GetTx: первый GetTxREQ + аларм ре-арма
        auto mf=getMetaFull();
        do_request_for_transactions(hb, mf);
        sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_GetTxREQ,
                  toRef(hb_hash.container), hb, 0.5, this));
        return true;
    }
    case timers::TIMER_GetTxREQ:
    {
        if(sync_in_progress)
        {
            return true;
        }
        auto &bt = l_blocks[prev_root_hash_Z()];
        THASH_id hb_hash;
        hb_hash.container=e->data->container;
        MsgData::HeartBeatREQ* hb=dynamic_cast<MsgData::HeartBeatREQ*> (e->cookie.get());
        if(!hb)
        throw CommonError("if(!hb)");

        if(!bt.is_GetTransactionRSP_m_quorum(hb_hash, root.get(), db_state.get()))
        {
            // кворума tx нет — ре-броадкаст GetTxREQ и ре-арм
            auto mf=getMetaFull();
            do_request_for_transactions(hb, mf);
            sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_GetTxREQ,
                      toRef(hb_hash.container), hb, 0.5, this));
        }
        // кворум есть — больше не шлём; дальше только финальное окно TIMER_GetTxRSP
        return true;
    }
    case timers::TIMER_GetTxRSP:
    {
        if(sync_in_progress)
        {
            return true;
        }
        auto &bt = l_blocks[prev_root_hash_Z()];
        THASH_id hb_hash;
        hb_hash.container=e->data->container;
        MsgData::HeartBeatREQ* hb=dynamic_cast<MsgData::HeartBeatREQ*> (e->cookie.get());
        if(!hb)
        throw CommonError("if(!hb)");
        if(bt.is_GetTransactionRSP_m_quorum(hb_hash, root.get(), db_state.get()))
        {
            do_start_block(hb);
            logNode("do_start_block();");
            bt.GetTxRSP_m[hb_hash].clear();
        }
        return true;
    }
    case TIMER_BlockDBStore:
    {
        

        auto r=dynamic_cast<MsgData::BlockDBStore*>(e->cookie.get());

        if(!r)
            throw CommonError("!r BlockDBStore");
        auto & bt= l_blocks[prev_root_hash_Z()];

        auto h=r->blockAccepted2REQ->blockAcceptedREQ->getHash();
        // logNode("alarm hash from coookie %s",h.str().c_str());
        THASH_id hd;
        hd.container=e->data->container;
    // logNode("alarm hash from data %s",hd.str().c_str());

        if(bt.is_BlockDiffValidateRSP_m_quorum(h,root.get(), db_state.get()))
        {

            auto mf=getMetaFull();
            TreeNode all_tree = filter_tree(mf->vne, bt.alive);
            broadcast_MsgEvent_via_broadcaster(r, all_tree);
            bt.BlockDiffValidateRSP_m[h].clear();


        }
        return true;
    }
    break;
    case TIMER_BlockDiffValidateREQ:
    {
        auto & bt= l_blocks[prev_root_hash_Z()];
        auto r=dynamic_cast<MsgData::BlockDiffValidateREQ*>(e->cookie.get());
        if(!r)
            throw CommonError("!r BlockDiffValidateREQ");
        auto mf=getMetaFull();
        auto h=r->blockAcceptedREQ->blockInfo->getHash();
        if(bt.is_ValidateBlockRSP_m_quorum(h,mf.get()))
        {
            TreeNode all_tree = filter_tree(mf->vne, bt.alive);
            broadcast_MsgEvent_via_broadcaster(r, all_tree);
            bt.ValidateBlockRSP_m[h].clear();

        }
        return true;
    }
    break;        

    default: logNode("unhandled alarm %d",e->tid);
    }

    return false;
}

bool Node::Service::handleEvent(const REF_getter<Event::Base> &e)
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
        case bcEventEnum::ClientMsg:
            return ClientMsg((const bcEvent::ClientMsg *)e.get());
        case bcEventEnum::AddTxREQ:
            return AddTxREQ(static_cast<const bcEvent::AddTxREQ *>(e.get()));

        case bcEventEnum::syncGetGranulesREQ:
            return syncGetGranulesREQ((const bcEvent::syncGetGranulesREQ *)e.get());
        case bcEventEnum::syncGetGranulesRSP:
            return syncGetGranulesRSP((const bcEvent::syncGetGranulesRSP *)e.get());
        case bcEventEnum::NodeMsgREQ:
            return NodeMsgREQ((const bcEvent::NodeMsgREQ *)e.get());
        case bcEventEnum::NodeMsgRSP:
            return NodeMsgRSP((const bcEvent::NodeMsgRSP *)e.get());
        case bcEventEnum::NodeNotRespondedREQ:
            return on_NodeNotResponded((const bcEvent::NodeNotRespondedREQ *)e.get());
        case bcEventEnum::ForwardTxREQ:
            return ForwardTxREQ((const bcEvent::ForwardTxREQ *)e.get());
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer *)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm *)e.get());
        case webHandlerEventEnum::RequestIncoming:
            return on_RequestIncoming((const webHandlerEvent::RequestIncoming *)e.get());
        case telnetEventEnum::CommandEntered:
            return on_CommandEntered((const telnetEvent::CommandEntered *)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService *)e.get());
        case bcEventEnum::ClientMsgReply:
            passEvent(e);
            return true;
        case httpEventEnum::RequestIncoming:
            return RequestIncoming(static_cast<const httpEvent::RequestIncoming *>(e.get()));
        case rpcEventEnum::IncomingOnAcceptor:
        {
            MUTEX_INSPECTOR;
            const rpcEvent::IncomingOnAcceptor *ev = static_cast<const rpcEvent::IncomingOnAcceptor *>(e.get());
            auto &IDA = ev->e->id;

            switch (IDA)
            {
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg *)ev->e.get());
            case bcEventEnum::AddTxREQ:
                return AddTxREQ(static_cast<const bcEvent::AddTxREQ *>(ev->e.get()));
            case bcEventEnum::ForwardTxREQ:
                return ForwardTxREQ(static_cast<const bcEvent::ForwardTxREQ *>(ev->e.get()));

            case bcEventEnum::syncGetGranulesREQ:
                return syncGetGranulesREQ((const bcEvent::syncGetGranulesREQ *)ev->e.get());
            case bcEventEnum::syncGetGranulesRSP:
                return syncGetGranulesRSP((const bcEvent::syncGetGranulesRSP *)ev->e.get());
            case bcEventEnum::NodeMsgREQ:
                return NodeMsgREQ((const bcEvent::NodeMsgREQ *)ev->e.get());
            case bcEventEnum::NodeMsgRSP:
                return NodeMsgRSP((const bcEvent::NodeMsgRSP *)ev->e.get());
            default:
                throw CommonError("unhabdled ev %d %s", IDA, iUtils->genum_name(IDA));
            }
        }
        break;
        case rpcEventEnum::IncomingOnConnector:
        {
            MUTEX_INSPECTOR;
            const rpcEvent::IncomingOnConnector *ev = static_cast<const rpcEvent::IncomingOnConnector *>(e.get());
            auto &IDC = ev->e->id;
            switch (IDC)
            {
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg *)ev->e.get());
            case bcEventEnum::AddTxREQ:
                return AddTxREQ(static_cast<const bcEvent::AddTxREQ *>(ev->e.get()));
            case bcEventEnum::ForwardTxREQ:
                return ForwardTxREQ(static_cast<const bcEvent::ForwardTxREQ *>(ev->e.get()));
            case bcEventEnum::syncGetGranulesREQ:
                return syncGetGranulesREQ((const bcEvent::syncGetGranulesREQ *)ev->e.get());
            case bcEventEnum::syncGetGranulesRSP:
                return syncGetGranulesRSP((const bcEvent::syncGetGranulesRSP *)ev->e.get());
            case bcEventEnum::NodeMsgREQ:
                return NodeMsgREQ((const bcEvent::NodeMsgREQ *)ev->e.get());
            case bcEventEnum::NodeMsgRSP:
                return NodeMsgRSP((const bcEvent::NodeMsgRSP *)ev->e.get());

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
        logNode("Node std::exception  %s", e.what());
    }
    XPASS;
    return false;
}
#include <regex>
static bool match(const std::string &re, const std::string &buf, std::vector<std::string> &tokens)
{
    MUTEX_INSPECTOR;
    std::regex rgx(re);
    std::smatch match;
    if (std::regex_search(buf, match, rgx))
    {
        tokens.clear();
        for (size_t i = 0; i < match.size(); i++)
        {
            tokens.push_back(match[i].str());
        }
        return true;
    }
    return false;
}
bool Node::Service::on_CommandEntered(const telnetEvent::CommandEntered *e)
{
    MUTEX_INSPECTOR;
    logNode("telnet command %s", e->command.c_str());
    std::vector<std::string> tokens;
    auto ds = "^ds$";
    auto go = "^go\\s+(.+)$";
    auto back = "^back$";
    if (match("^make_snapshot$", e->command, tokens)) {
        auto db = dynamic_cast<CDatabaseRocksdb*>(db_state.get());
        if (!db) {
            sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "make_snapshot: not rocksdb\n", this));
            return true;
        }
        std::string ts = std::to_string(time(NULL));
        std::string snap_path = db_name + "_snapshot_" + ts;
        rocksdb::Checkpoint* cp = nullptr;
        rocksdb::Status s = rocksdb::Checkpoint::Create(db->db_ptr, &cp);
        if (s.ok()) s = cp->CreateCheckpoint(snap_path);
        delete cp;
        sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId,
            (std::string)"make_snapshot: " + (s.ok() ? "ok " : "err ") + snap_path + "\n", this));
        return true;
    }
    if (match(ds, e->command, tokens))
    {
    }
    if (match(go, e->command, tokens))
    {

        sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "if(match(go, e->command, tokens)) " + std::to_string(tokens.size()) + "\n", this));
        if (tokens.size() == 2)
        {
            sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "if(tokens.size()==2)\n", this));
            telnet_data_path.push_back(tokens[1]);
        }
    }
    if (match(back, e->command, tokens))
    {
        telnet_data_path.pop_back();
    }

    sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "NodeService received command: " + e->command + "\n", this));

    return true;
}

Node::Service::~Service()
{

}

bool Node::Service::on_NodeNotResponded(const bcEvent::NodeNotRespondedREQ *e)
{
    unresponsive_nodes.insert(e->node);
    return true;
}

bool Node::Service::NodeMsgREQ(const bcEvent::NodeMsgREQ *m)
{
    if(sync_in_progress)
    {
        logNode("skipped: sync in progress %s %d",__FILE__,__LINE__);
    }

    auto &s=filter_NodeMsgREQ[m->node_signer][m->node_start_timestamp];
    while(s.size()>100)
    {
        s.erase(s.begin());
    }
    if(s.count(m->seqId2))
    {
        logNode("filter_NodeMsgREQ.count(m->seqId2) %lld",m->seqId2);
        return true;
    }
    s.insert(m->seqId2);
    if(!sync_in_progress)
    {
        auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(),m->node_signer);
        if(!n.valid())
            return true;
        if (!verify_ed_pk(n->get_ed_pk(), m->signature, blake2b_hash(m->msg_payload)))
        {
            logNode("verify failed 11");
            return true;
        }
    }
    inBuffer in(m->msg_payload);
    auto id = in.get_PN();

    REF_getter<MsgData::Base> msg = msgFactory.create(id);
    msg->unpack(in);

    if(!sync_in_progress)
    {
        switch (msg->type)
        {
        case msgid::GetTxREQ:
            return GetTxREQ(static_cast<const MsgData::GetTxREQ *>(msg.get()), m->node_signer, m->route);
        case msgid::PingREQ:
            return PingREQ(static_cast<const MsgData::PingREQ *>(msg.get()), m->node_signer, m->route);
        case msgid::ValidateBlockREQ:
            // last_activity_time=iUtils->getNow().v;
            return ValidateBlockREQ(static_cast<const MsgData::ValidateBlockREQ *>(msg.get()), m->node_signer, m->route);
        case msgid::BlockAccepted2REQ:
            // last_activity_time=iUtils->getNow().v;
            return BlockDBStore(static_cast<const MsgData::BlockDBStore *>(msg.get()), m->node_signer, m->route);
        // case msgid::ConfirmLeaderREQ:
        //     return ConfirmLeaderREQ(static_cast<const MsgData::ConfirmLeaderREQ *>(msg.get()), m->node_signer, m->route);
        case msgid::BlockDiffValidateREQ:
            return BlockDiffValidateREQ(static_cast<const MsgData::BlockDiffValidateREQ *>(msg.get()), m->node_signer, m->route);
        case msgid::BlockDBStore:
            return BlockDBStore(static_cast<const MsgData::BlockDBStore *>(msg.get()), m->node_signer, m->route);
        // case msgid::LcEnvelopeREQ:
        // {
        //     bool nc=true;
        //     return LcEnvelopeREQ(static_cast<const MsgData::LcEnvelopeREQ *>(msg.get()), m->node_signer, m->route,&nc);

        // }

        default:
            throw CommonError("unjandled3 MsgData %s", msgName(msg->type));
        }

    }
    else
    {
        switch (msg->type)
        {
        // case msgid::BlockDiffValidateREQ:
        //     return BlockDiffValidateREQ_sync(static_cast<const MsgData::BlockDiffValidateREQ *>(msg.get()), m->node_signer, m->route);
        case msgid::GetTxREQ:
        case msgid::ValidateBlockREQ:
        case msgid::BlockAccepted2REQ:
        case msgid::ConfirmLeaderREQ:
        case msgid::BlockDBStore:

        default:
            logNode("skipped in sync mode %s", msgName(msg->type));
        }

    }

    return true;
}

bool Node::Service::NodeMsgRSP(const bcEvent::NodeMsgRSP *m)
{
    if (m->route.size())
    {
        passEvent(m);
        return true;
    }

    auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(),m->node_signer);
    if(!n.valid())
    { 
        logNode("NodeMsgRSP from unknown node %s, ignore", m->node_signer.container.c_str()); 
        return true; 
    }
    if (!verify_ed_pk(n->get_ed_pk(), m->signature, blake2b_hash(m->msg_payload)))
    {
        logNode("verify failed @4");
        return true;
    }
    inBuffer in(m->msg_payload);
    auto id = in.get_PN();

    REF_getter<MsgData::Base> ee = msgFactory.create(id);
    ee->unpack(in);

    switch (id)
    {
    case msgid::GetTxRSP:
        return GetTxRSP(static_cast<const MsgData::GetTxRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::PingRSP:
        return PingRSP(static_cast<const MsgData::PingRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::ValidateBlockRSP:
        return ValidateBlockRSP(static_cast<const MsgData::ValidateBlockRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::BlockDiffValidateRSP:
        return BlockDiffValidateRSP(static_cast<const MsgData::BlockDiffValidateRSP *>(ee.get()), m->node_signer, m->route);
    default:
        throw CommonError("unhandled22 p020 %s", msgName(id));
        break;
    }

    return true;
}

Node::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins, Mode m)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins),
      iInstance(ins),
      mode_(m)
{
    MUTEX_INSPECTOR;
    node_start_timestamp=iUtils->getNow().v;

    if (mode_ == Lite) {
        // Lite: без сети, без ключей. Только uplink + локальный кеш.
        uplink_http_addr_ = ins->getConfig()->get_string("node_http_addr", "", "full node /granule addr (required for lite)");
        db_name = ins->getConfig()->get_string2("db_name", "db/nl0", "lite db name");
        return;
    }

    // Full
    rpc_addr = ins->getConfig()->get_tcpaddr("rpc_addr", "127.0.0.1:2345", "rpc address(es) of node ex: ip:port,ip2:port2");
    web_addr = ins->getConfig()->get_tcpaddr("web_addr", "127.0.0.1:2347", "web address(es) of node ex: ip:port,ip2:port2");

    std::string bls_sk_hex = iInstance->getConfig()->get_string("my_sk_bls", "", "BLS secret (hex, inline)");
    std::string ed_sk_hex  = iInstance->getConfig()->get_string("my_sk_ed",  "", "Ed secret (hex, inline)");
    if (!bls_sk_hex.empty()) my_sk_bls.deserializebase16Str(bls_sk_hex);
    else                     my_sk_bls.deserializebase16Str(getenv2(my_sk_bls_env_key));
    if (!ed_sk_hex.empty())  my_sk_ed = base16::decode(ed_sk_hex);
    else                     my_sk_ed = base16::decode(getenv2(my_sk_ed_env_key));

    this_node_name.container = ins->getConfig()->get_string("this_node_name", "n0", "registered name of node");
    db_name = ins->getConfig()->get_string2("db_name", "grain", "db name");
    http_granule_addr_ = ins->getConfig()->get_tcpaddr("http_granule_addr", "127.0.0.1:3301", "HTTP /granule addr (full mode)");
}

bool Node::Service::on_RequestIncoming(const webHandlerEvent::RequestIncoming *)
{
    MUTEX_INSPECTOR;
    return true;
}
void registerNodeService(const char *pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if (pn)
    {
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::Node, "Node", getEvents_nodeService());
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::NodeLite, "NodeLite", getEvents_nodeService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::Node, Node::Service::constructFull, "Node");
        iUtils->registerService(ServiceEnum::NodeLite, Node::Service::constructLite, "NodeLite");
        regEvents_nodeService();
    }
    XPASS;
}

bool Node::Service::RequestIncoming(const httpEvent::RequestIncoming* e)
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
    int r = db_state->getGranule(addr_bin, &body);
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

void Node::Service::do_request_for_transactions(MsgData::HeartBeatREQ* hb, const REF_getter<BlockMetaFull>& meta)
{
    MUTEX_INSPECTOR;

    REF_getter<MsgData::GetTxREQ> rt = new MsgData::GetTxREQ();
    rt->hb = hb;
    auto n=cells::getNodeNoCreateConst(root.get(), db_state.get(), this_node_name);
    if(!n.valid())
        throw CommonError("if(!n) @330944");
    rt->ip=n->get_ip();
    // li.request_for_transactions_time = iUtils->getNow().v;

    // GetTx только к живым (после стадии пинга bt.alive собран)
    auto &bt = l_blocks[prev_root_hash_Z()];
    TreeNode tx_tree = filter_tree(meta->vne, bt.alive);
    broadcast_MsgEvent_via_broadcaster(rt.get(), tx_tree);
}

THASH_id Node::Service::execute_block(b_params &b,  const REF_getter<MsgData::HeartBeatREQ> &lc)
{
    MUTEX_INSPECTOR;
    if (getenv("MTJS_TREE_DUMP"))
        logNode("[TRACE execute_block] begin txs=%zu", (size_t)b.validateBlockREQ->transaction_bodies.size());
    // logNode("[execute_block] sender %s",b.)
    {
        auto st = cells::getStateNoCreate(root.get(), db_state.get(),NULL,b.validateBlockREQ->block_timestamp);
        auto bh=lc->new_epoch;
        if (st.valid()) {
            
            st->block_height=bh;
        }
    }
    DBG(logNode("execute_block: tx_count=%zu", (size_t)b.validateBlockREQ->transaction_bodies.size()));
    // Nonce повышаем после всех tx: внутри цикла u взят до execute_transaction,
    // а откат (unpack_mx) заменяет data-объект — и incNonce уходит в отвязанный объект.
    std::vector<ADDRESS_id> nonce_bumps;
    // nonce из блока применяется после цикла, поэтому в пределах блока
    // второй tx того же сендера должен видеть уже учтённый nonce.
    std::map<ADDRESS_id,int64_t> pending_nonce;
    for (int ti = 0; ti < b.validateBlockREQ->transaction_bodies.size(); ti++)
    {
        MUTEX_INSPECTOR;
        resetBlockExpired(b.validateBlockREQ->heart_beat->node_leader);
        std::optional<std::string> t_err;
        auto tt=b.validateBlockREQ->transaction_bodies[ti];
        auto tx_hash=tt->getHash();
        auto &tj = tt->tx_body;
        yyjson_val *jroot2 = yyjson_doc_get_root(tt->doc);
        std::string sender_hex;
        yyjson_val *sv2 = jroot2 ? yyjson_obj_get(jroot2, "sender") : nullptr;
        if (sv2 && yyjson_is_str(sv2)) sender_hex = yyjson_get_str(sv2);

        int64_t key_id = -1;
        yyjson_val *ki2 = jroot2 ? yyjson_obj_get(jroot2, "key_id") : nullptr;
        if (ki2 && yyjson_is_num(ki2)) key_id = yyjson_get_sint(ki2);

        ADDRESS_id senderAddress;
        if (!sender_hex.empty()) {
            senderAddress.addr = base16::decode(sender_hex);
            auto uu = cells::getAddressStateNoCreateConst(root.get(), db_state.get(),senderAddress);
            if (!uu.valid()) { t_err = "sender address not found"; }
            else if (key_id < 0) { t_err = "key_id required"; }
            else {
                auto k = uu->getKey((uint64_t)key_id);
                if (!k) { t_err = "key_id not found"; }
                else if (!verify(*k, tt->sig_ed_bin, tt->getHash().container)) { t_err = "verify failed"; }
            }
        } else {
            t_err= "sender address not specified";
        }
        if (!t_err)
        {
            MUTEX_INSPECTOR;
            auto u = cells::getAddressStateNoCreateConst(root.get(), db_state.get(),senderAddress);
            if (!u.valid())
            {
                t_err = "sender invalid";
                logNode("sender invalid");
            }
            if (!t_err)
            {
                int64_t nonce;
                auto err=tt->getNonce(nonce);
                if(err) throw CommonError(*err);
                if (u->getNonce() + pending_nonce[senderAddress] != nonce)
                {
                    logNode("invalid nonce, expected %lld got %lld", u->getNonce() + pending_nonce[senderAddress], nonce);
                    t_err = "invalid nonce";

                }
                if (!t_err)
                {
                    MUTEX_INSPECTOR;
                    if(!lc.valid())
                        logNode("if(!lc.valid()) AA");
                    if(!lc.valid())
                        logNode("if(!lc->heart_beat.valid()) AA");
                    // t_params t;
                    // t.senderAddress=senderAddress;
                    if (getenv("MTJS_TREE_DUMP"))
                        logNode("[TRACE tx-begin] tx=%s", base16::encode(tt->getHash().container).substr(0,12).c_str());
                    t_err=execute_transaction(tt->getHash(),  b, senderAddress, tt, epoch_current());
                    if (getenv("MTJS_TREE_DUMP"))
                        logNode("[TRACE tx-end] tx=%s err=%d", base16::encode(tt->getHash().container).substr(0,12).c_str(), (int)(t_err ? 1 : 0));
                    nonce_bumps.push_back(senderAddress);
                    pending_nonce[senderAddress]++;
                    // logNode("execute_block: after tx att_blockRoot_children=%d", (int)b.att_data->blockRoot.children.size());

                }
            }
        }
        if (!t_err)
            b.emit_tx(tx_hash, "result", R"({"success":true})");
        else
            b.emit_tx(tx_hash, "error", R"({"error":"%s"})", t_err->c_str());
    }

    // Фаза block-level изменений — вне tx: nonce, затем награды.
    for (auto& a : nonce_bumps)
    {
        auto u = cells::getAddressStateNoCreate(root.get(), db_state.get(), a, NULL, b.validateBlockREQ->block_timestamp);
        if (!u.valid()) { logNode("execute_block: nonce bump: address not found"); continue; }
        u->incNonce();
    }
    calc_fee_rewards_nodes(b, lc);
    // Отключаем ноды, не ответившие на прошлый блок (missed_rounds > missed_rounds_max).
    // Детерминировано: тот же вход prev_block->node_validators на всех нодах.
    // Вызов ДО финального merkle, чтобы enabled/missed_rounds попали в root-хеш блока.
    {
        // ВАЖНО: node_validators — это только комитет (подписавшие). Ноды вне
        // комитета живые, но в node_validators не входят — их нельзя считать
        // «пропустившими». Правильный вход — node_diff_validators (все, кто
        // ответил на BlockDiffValidateREQ).
        if (prev_block.valid()) {
            std::set<NODE_id> prev_validators(prev_block->node_diff_validators.begin(),
                                              prev_block->node_diff_validators.end());
            fix_missed_rounds(prev_validators, NULL, b.validateBlockREQ->block_timestamp);
        }
    }
    auto rh=proceed_merkle_on_transaction_pool_hashers(root,db_to_save_Z);
    return rh;
}
void Node::Service::calc_fee_rewards_nodes(b_params &b, const REF_getter<MsgData::HeartBeatREQ> &lc)
{
    MUTEX_INSPECTOR;

    double total_staked=0;
    auto nn=cells::getAllNodesConst(root.get(), db_state.get());
    auto local_prev_block=prev_block;
    std::set<NODE_id> ns;
    if(local_prev_block.valid())
    {
        for(auto& z:local_prev_block->blockAcceptedREQ->node_validators)
        {
            ns.insert(z);
            auto n=cells::getNodeNoCreateConst(root.get(), db_state.get(),z);
            if(!n.valid())
                throw CommonError("if(!n.valid()) --@34");

            total_staked+=n->get_full_stake();
        }
        for(auto& z:local_prev_block->blockAcceptedREQ->node_validators)
        {
            auto n=cells::getNodeNoCreateConst(root.get(), db_state.get(),z);
            if(!n.valid())
                throw CommonError("if(!n.valid())00@@");

            auto portion=n->get_full_stake()*b.node_rewards/total_staked;
            auto u = cells::getAddressStateNoCreate(root.get(), db_state.get(),n->get_owner(), NULL, b.validateBlockREQ->block_timestamp);
            if (u.valid()) {
                {
                    
                    u->balance += portion;
                }
            } else {
                // owner не найден — аномалия (node_create owner должен быть)
                logNode("calc_fee_rewards: owner not found for node %s (skip reward %lld)",
                        z.container.c_str(), (long long)portion);
            }
            b.emit_block("reward",R"({"node":"%s","fee":"%s"})",z.container.c_str(),std::to_string(portion).c_str());
        }

    }


    b.emit_block("total_fee",R"({"fee":"%s"})",std::to_string(b.node_rewards).c_str());

}

#include <stdlib.h>
bool Node::Service::isNodeGreater(const NODE_id &nodeLeft, const NODE_id &nodeRight, const REF_getter<BlockMetaFull>& m)
{
    MUTEX_INSPECTOR;
    auto itL=m->position_of_node.find(nodeLeft);
    if(itL == m->position_of_node.end())
        throw CommonError("if(itL == m->position_of_node.end()) %s", nodeLeft.container.c_str());
    auto itR=m->position_of_node.find(nodeRight);
    if(itR == m->position_of_node.end())
        throw CommonError("if(itR == m->position_of_node.end()) %s", nodeRight.container.c_str());
    return itL->second < itR->second;
}
bool Node::Service::verify_block_all(const REF_getter<MsgData::BlockAccepted2REQ> &lc)
{
    /// проверка сертификата лидера
    if(!lc.valid())
        return false;
    {
        MUTEX_INSPECTOR;
        std::vector<blst_cpp::PublicKey> agg_pk;

        int64_t stake=0;
        for (auto &z : lc->node_diff_validators)
        {
            auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(),z);
            if(!n.valid())
                throw CommonError("if(!n.valid()) #00");
            agg_pk.push_back(n->get_bls_pk());
            stake += n->get_full_stake();
        }
        int64_t full_stake=0;
        auto ls=cells::getAllNodesConst(root.get(), db_state.get());
        for(auto& z:ls )
        {
            if(z->isEnabled())
            {
                full_stake+=z->get_full_stake();
            }
        }

        if ((stake * 100) / full_stake < QUORUM)
        {
            logNode("this quorum %lld",(stake * 100) / full_stake);
            logNode("verify lc quorum failed %lld %lld",stake,full_stake);
            return false;
        }
        if (!lc->agg_diff_sig.verify(agg_pk, lc->blockAcceptedREQ->getHash().container))
        {
            logNode("verify lc - sign invalid");
            return false;
        }
    }

    return true;
}

bool Node::Service::ForwardTxREQ(const bcEvent::ForwardTxREQ *e)
{
    MUTEX_INSPECTOR;
    auto h=e->tx->getHash();
    transaction_pool_of_leader.insert_or_assign(h,e->tx);
    return true;
}

void print_exception(JSIContext* ctx);

std::optional<std::string> Node::Service::execute_tx_commands(b_params &b, t_params& t,
        yyjson_val * j_tx,std::set<REF_getter<data_base>> * deleted_granules)
{
    MUTEX_INSPECTOR;
    if(!yyjson_is_arr(j_tx))
        return "command list must be json array";

    yyjson_arr_iter iter;
    yyjson_arr_iter_init(j_tx, &iter);

    uint32_t index = 0;
    yyjson_val* item;
    {
        MUTEX_INSPECTOR;
        if(t.gasUsed>t.gasLimitGas) return "gas exceeds limit";
        while ((item = yyjson_arr_iter_next(&iter)))
        {
            MUTEX_INSPECTOR;
            bool err=false;
            yyjson_val* contract = yyjson_obj_get(item, "contract");
            yyjson_val* method = yyjson_obj_get(item, "method");
            yyjson_val* params = yyjson_obj_get(item, "params");
            if(contract==NULL) return "'contract' field required";
            if(method==NULL) return "'method' field required";
            if(params==NULL) return "'params' field required";
            if(!yyjson_is_str(contract)) return "'contract' must be string type";
            if(!yyjson_is_str(method)) return "'method' must be string type";
            if(!yyjson_is_obj(params)) return "'params' must be object type";
            std::string contract_str=yyjson_get_str(contract);
            std::string method_str=yyjson_get_str(method);

            if (contract_str == "root")
            {
                MUTEX_INSPECTOR;
                // logNode("execute_tx_commands: contract=root method=%s", method_str.c_str());
                std::optional<std::string> err;
                auto meth=method_str;
                if (meth == "mint") err = execute_mint(params, b, t, index);
                else if (meth == "transfer") err = execute_transfer(params, b,t, index);
                else if (meth == "node_create") err = execute_node_create(params,b, t, index);
                else if (meth == "node_update") err = execute_node_update(params, b,t, index);
                else if (meth == "node_stake") err = execute_node_stake(params, b,t, index);
                else if (meth == "node_unstake") err = execute_unstake_node(params, b,t, index);
                else if (meth == "node_enable") err = execute_node_enable(params, b,t, index);
                else if (meth == "node_disable") err = execute_node_disable(params, b,t, index);
                else if (meth == "contract_deploy") err = execute_contract_deploy(params,b, t, index);
                else if (meth == "contract_update") err = execute_contract_update(params, b,t, index);
                else if (meth == "vote") err = execute_vote(params, b,t, index);
                else if (meth == "gc") err = execute_gc(params, b,t, index, db_to_save_Z);
                else if (meth == "compact") err = execute_compact(params, b, t, index);
                else if (meth == "create_address")      err = execute_create_address(params, b, t, index);
                else if (meth == "add_address_key")     err = execute_add_address_key(params, b, t, index);
                else if (meth == "remove_address_key")  err = execute_remove_address_key(params, b, t, index);                
                else return "unhandled method '"+method_str+"' for root contract";
                if(err) logNode("err: %s",err->c_str());
                if (err) return err;
            }
            else
            {
                MUTEX_INSPECTOR;
                // JSI-контракт (gym.js и т.п.)
                CONTRACT_id c; c.container = contract_str;
                auto cc = contractHost->get_contract(root.get(), c);
                if (!cc.valid()) return "contract not found: " + contract_str;

                // params yyjson -> json
                std::string params_str = yyjson_val_write(params, 0, nullptr);
                nlohmann::json params_json;
                if (!params_str.empty()) {
                    try { params_json = nlohmann::json::parse(params_str); }
                    catch (const std::exception& e) { return std::string("params parse error: ") + e.what(); }
                } else params_json = nlohmann::json::object();

                CallStack call_stack;

                // внешний CallContext — чтобы call_method докладывал emits в него (parent->emits)
                CallContext ext_ctx;
                ext_ctx.contract = cc.get();
                ext_ctx.host = contractHost.get();
                ext_ctx.call_stack = &call_stack;
                ext_ctx.value_ptr = &t.value;   // текущий value (нода)
                ext_ctx.deleted_granules=deleted_granules;
                void* prev_ctx = JSI_GetContextOpaque(cc->ctx);
                JSI_SetContextOpaque(cc->ctx, &ext_ctx);

                NodeMsgSource src(&t);
                CONTRACT_id caller;
                JSIValue result = cc->call_method(root.get(), method_str, params_json, &src, 
                                                           &call_stack, contractHost.get(), t.roll, caller, & b.fees, deleted_granules,b.validateBlockREQ->block_timestamp);

                JSI_SetContextOpaque(cc->ctx, prev_ctx);

                if (JSI_IsException(result)) {
                    std::string emsg;
                    JSIValue exc = JSI_GetException(cc->ctx);
                    if (!JSI_IsUndefined(exc)) {
                        const char* s = JSI_ToCString(cc->ctx, exc);
                        if (s) { emsg = s; JSI_FreeCString(cc->ctx, s); }
                        JSI_FreeValue(cc->ctx, exc);
                    }
                    JSI_FreeValue(cc->ctx, result);
                    return "contract call failed: " + contract_str + "." + method_str + (emsg.empty() ? "" : ": " + emsg);
                }
                // logNode("execute_tx_commands: contract=%s method=%s index=%d ext_ctx.emits.size=%d",
                    // contract_str.c_str(), method_str.c_str(), (int)index, (int)ext_ctx.emits.size());
                for (auto& ev : ext_ctx.emits)
                {
                    DBG(logNode("  emit: %s = %s", ev.name.c_str(), ev.value.c_str()));
                }

                JSI_FreeValue(cc->ctx, result);

                // эмиты текущей команды → b.emit_command(tx_id, seqId=index, name, value_json)
                for (auto& ev : ext_ctx.emits) {
                    b.emit_command(t.tx_id, (int)index, ev.name, "%s", ev.value.c_str());
                }
            }
            ++index;
        }
    }
    return std::nullopt;
}
std::optional<std::string> token_2_gas(int64_t tokens, int64_t priority,
                                       int64_t gas_per_token_base, int64_t& result)
{
    if (priority < 0 || priority > 99)
        return "priority must be 0..99";
    if (tokens < 0)
        return "tokens must be >= 0";
    if (gas_per_token_base <= 0)
        return "gas_per_token must be > 0";

    int64_t gas_per_token = gas_per_token_base * (100 - priority) / 100;   // base=1000 → 1000..10
    if (gas_per_token <= 0)
        return "gas_per_token <= 0";

    __int128 gasLimit_gas = (__int128)tokens * gas_per_token;
    if (gasLimit_gas > INT64_MAX)
        return "gasLimit overflow";
    result = (int64_t)gasLimit_gas;
    return std::nullopt;
}

std::optional<std::string> gas_2_token(int64_t gasUsed, int64_t priority,
                                       int64_t gas_per_token_base, int64_t& result)
{
    if (priority < 0 || priority > 99)
        return "priority must be 0..99";
    if (gasUsed < 0)
        return "gasUsed must be >= 0";
    if (gas_per_token_base <= 0)
        return "gas_per_token must be > 0";

    int64_t gas_per_token = gas_per_token_base * (100 - priority) / 100;   // base=1000 → 1000..10
    if (gas_per_token <= 0)
        return "gas_per_token <= 0";

    // округление вверх: ceil(gasUsed / gas_per_token); минимум 1 токен (анти-спам)
    int64_t tokens = (gasUsed + gas_per_token - 1) / gas_per_token;
    if (tokens < 1) tokens = 1;

    result = tokens;
    return std::nullopt;
}
std::optional<std::string> Node::Service::execute_transaction(const THASH_id &tx_id, b_params &b, const ADDRESS_id &senderAddress,
        const REF_getter<MsgData::TX> &tx, int64_t epoch)
{
    MUTEX_INSPECTOR;
    Rollback roll;
    std::set<REF_getter<data_base>> deleted_granules;
    // ▼▼▼ INSERT: bc_state tx_count++ (TX попала в блок; NULL — не rollback)
    {
        auto st = cells::getStateNoCreate(root.get(), db_state.get(), &roll,b.validateBlockREQ->block_timestamp);
        if (!st.valid()) 
            throw CommonError("if (!st.valid()) ");
        st->tx_count++;
    }
    // ▲▲▲    
    int64_t gasLimitTokens = 0, priority = 0, value = 0;
    yyjson_val *jroot = yyjson_doc_get_root(tx->doc);
    yyjson_val *j_tx = yyjson_obj_get(jroot, "tx");
    if (!j_tx)
        throw CommonError("if(!j_tx)");
    auto err = yy_get_int64_t(jroot, "value", value);
    if (!err) err = yy_get_int64_t(jroot, "gasLimit", gasLimitTokens);   // в токенах
    if (!err) err = yy_get_int64_t(jroot, "priority", priority);
    if (err)
    {
        b.emit_tx(tx_id, "error", R"({"error":"%s"})", err->c_str());
        return err;
    }

    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
    int64_t gas_per_token_base = b.fees.gas_per_token;

    int64_t gasLimit_gas = 0;
    if (auto e = token_2_gas(gasLimitTokens, priority, gas_per_token_base, gasLimit_gas))
    {
        // b.emit_tx(tx_id, "error", R"({"error":"%s"})", e->c_str());
        return e;
    }

    t_params t(db_state);
    t.senderAddress = senderAddress;
    t.tx = tx;
    t.epoch = epoch;
    t.tx_id = tx_id;
    t.roll = &roll;
    t.value = value;
    t.gasLimitGas = gasLimit_gas;
    t.deleted_granules=&deleted_granules;
    t.b=&b;
    {
        int64_t n = 0;
        if (!tx->getNonce(n)) t.nonce = (int64_t)n;
    }
    {
        yyjson_val *cid = yyjson_obj_get(jroot, "chainId");
        if (cid && yyjson_is_num(cid)) t.chain_id = (int64_t)yyjson_get_sint(cid);
        else if (cid && yyjson_is_str(cid)) t.chain_id = (int64_t)std::atoll(yyjson_get_str(cid));
    }
    t.tx_index = 0; // TBD

    auto uu = cells::getAddressStateNoCreate(root.get(), db_state.get(),senderAddress, t.roll,b.validateBlockREQ->block_timestamp);
    if (!uu.valid())
        throw CommonError("if(!uu.valid())");

    int64_t bal0 = 0;
    {
        
        bal0 = uu->balance;
        if (uu->balance < gasLimitTokens + value)
        {
            // b.emit_tx(tx_id, "error", R"({"error":"not enough funds to reserve gasLimit+value"})");
            return "not enough funds to reserve gasLimit+value";

        }
    }

    root->calc_tree_hash(db_to_save_Z);

    // helper: списать газ (в токенах) с баланса и вернуть ошибку (если priority/gas невалидны)
    auto charge_gas = [&](int64_t gas_charged, int64_t& spent) -> std::optional<std::string> {
        if (auto e = gas_2_token(gas_charged, priority, gas_per_token_base, spent))
            return e;
        
        uu->balance -= spent;
        b.node_rewards += spent;
        return std::nullopt;
    };

    // helper: rollback + emit error + списать газ + лог; возвращает сообщение
    auto fail = [&](const std::string& msg, int64_t gas_charged) -> std::optional<std::string> {
        t.gasUsed += t.roll->size() * b.fees.gas_per_byte;
        t.rollback();
        // uu указывал на data_base до отката; rollback (unpack_mx) заменил объект,
        // поэтому перечитываем сендера ЗАНОВО — иначе газ спишется в отвязанный объект.
        uu = cells::getAddressStateNoCreate(root.get(), db_state.get(), senderAddress, t.roll, b.validateBlockREQ->block_timestamp);
        // b.emit_tx(t.tx_id, "error", R"({"error":"%s"})", msg.c_str());
        int64_t spent = 0;
        if (auto e = charge_gas(gas_charged, spent))
            return e;
        int64_t bal1 = 0;
        {  bal1 = uu->balance; }
        #ifdef DEBUG
        logNode("[t] tx=%s value=%lld gasLimitTokens=%lld priority=%lld gasLimit_gas=%lld gpt=%lld "
                "gasUsed=%lld rollback=%d total=%lld spentTokens=%lld value_spent=0 bal %lld->%lld status=err msg=\"%s\"",
                base16::encode(tx_id.container).c_str(),
                (long long)value, (long long)gasLimitTokens, (long long)priority,
                (long long)gasLimit_gas,
                (long long)(gas_per_token_base * (100 - priority) / 100),
                (long long)t.gasUsed, (int)t.roll->size(), (long long)t.gasUsed,
                (long long)spent,
                (long long)bal0, (long long)bal1, msg.c_str());
#endif
                return msg;
    };
    // std::set<REF_getter<data_base>> deteled_granules;
    err = execute_tx_commands(b, t, j_tx, &deleted_granules);
    if (err)
        return fail(*err, t.gasUsed);

    if (t.value < 0)
        return fail("value exceeds limit", t.gasUsed);

    if (t.gasUsed > gasLimit_gas)
        return fail("gas exceeds limit", gasLimit_gas);

    _db_to_save db_dump;
    for(auto& z: deleted_granules)
    {
        deleteGranule(z.get(),&roll,db_dump,t.b->validateBlockREQ->block_timestamp);
    }
    root->calc_tree_hash(db_dump);
    // t.gasUsed += db_dump.size();
    t.gasUsed += db_dump.size_leaves() * b.fees.gas_per_byte;
    if (t.gasUsed > gasLimit_gas)
        return fail("gas exceeds limit", gasLimit_gas);

    int64_t spent = 0;
    if (auto e = charge_gas(t.gasUsed, spent))
        return e;

    {
        
        uu->balance -= value - t.value;
    }
    int64_t bal1 = 0;
    {  bal1 = uu->balance; }

    root->calc_tree_hash(db_dump);
    if (getenv("MTJS_TREE_DUMP")) {
        for (auto& z : db_dump.cells)
            logNode("[TRACE tx-diff] tx=%s id=%s h=%s",
                    base16::encode(tx_id.container).substr(0,12).c_str(),
                    base16::encode(z.first).substr(0,16).c_str(),
                    base16::encode(blake2b_hash(z.second).container).substr(0,16).c_str());
    }
    db_to_save_Z.add(db_dump);
#ifdef DEBUG
    logNode("[t] tx=%s value=%lld gasLimitTokens=%lld priority=%lld gasLimit_gas=%lld gpt=%lld "
            "gasUsed=%lld db_dump=%d total=%lld spentTokens=%lld value_spent=%lld bal %lld->%lld status=ok",
            base16::encode(tx_id.container).c_str(),
            (long long)value, (long long)gasLimitTokens, (long long)priority,
            (long long)gasLimit_gas,
            (long long)(gas_per_token_base * (100 - priority) / 100),
            (long long)(t.gasUsed - db_dump.size()), (int)db_dump.size(), (long long)t.gasUsed,
            (long long)spent, (long long)(value - t.value),
            (long long)bal0, (long long)bal1);
#endif
    return std::nullopt;
}

inline uint64_t read_uint64(const uint8_t* data) {
    return (static_cast<uint64_t>(data[0]) << 56) |
           (static_cast<uint64_t>(data[1]) << 48) |
           (static_cast<uint64_t>(data[2]) << 40) |
           (static_cast<uint64_t>(data[3]) << 32) |
           (static_cast<uint64_t>(data[4]) << 24) |
           (static_cast<uint64_t>(data[5]) << 16) |
           (static_cast<uint64_t>(data[6]) << 8)  |
           (static_cast<uint64_t>(data[7]));
}
void Node::Service::sync_watchdog_arm()
{
    sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::ALARM_SYNC_WATCHDOG,NULL,NULL,SYNC_WATCHDOG_SEC,this));
}
void Node::Service::sync_watchdog_stop()
{
    sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(timers::ALARM_SYNC_WATCHDOG,NULL,this));
}
REF_getter<const bc_node> Node::Service::current_leader()
{
    if(sync_in_progress)
    {
        logNode("current_leader: sync in progress %s %d",__FILE__,__LINE__);
    }
    auto mf=getMetaFull();
    if(!mf.valid() || mf->vne.empty())
        return nullptr;
    const auto& leaders = mf->committee_vne.empty() ? mf->vne : mf->committee_vne;

    int64_t lastBlockTime=0;
    if(prev_block.valid())
        lastBlockTime=prev_block->blockAcceptedREQ->blockInfo->block_timestamp.v;
    const auto now=iUtils->getNow().v;
    if(now<lastBlockTime)
        return nullptr;   // часы ушли назад относительно блока — слот не определён

    const int64_t since=now-lastBlockTime;
    // правая граница слота — мёртвая зона: лидера нет
    if(since%SLOT_MKS>=SLOT_MKS-NO_LEADER_FRAME_MKS)
        return nullptr;

    const NODE_id& name=leaders[(since/SLOT_MKS)%leaders.size()].name;
    return cells::getNodeNoCreateConst(root.get(),db_state.get(),name);
}
REF_getter<BlockMetaFull> Node::Service::getMetaFull()
{
    MUTEX_INSPECTOR;
    // Периодически пересматриваем unresponsive: нода могла вернуться после sync.
    // Без этого исключение одностороннее — вернувшаяся нода не получит броадкаст
    // и не сможет доказать, что жива.
    {
        int64_t e = epoch_current();
        if (unresponsive_recheck_epoch < 0 || e - unresponsive_recheck_epoch >= 10)
        {
            unresponsive_nodes.clear();
            unresponsive_recheck_epoch = e;
        }
    }
    // int round=hb->round;
    auto b=prev_root_hash_Z();
    // auto round=ti_ / HB_TIME_WINDOW;
    auto it=block_meta_full.find(b);
    if(it!=block_meta_full.end())
    {
        return it->second;
    }

    REF_getter<BlockMetaFull> m=new BlockMetaFull();
    block_meta_full[b]=m;
    // auto an=cells::getAllNodes(root.get(),db_state.get());
    auto aan=cells::getNodeListNoCreateConst(root.get(),db_state.get());
    if(!aan.valid())
        return nullptr;
    int live_nodes=0;
    std::map<NODE_id,REF_getter<const bc_node>> nnn;
    for(auto& z: aan->list)
    {
        auto n=cells::getNodeNoCreateConst(root.get(),db_state.get(),z);
        if(!n.valid())
            return nullptr;
        nnn[z]=n;
    }

    auto v=cells::getValuesNoCreateConst(root.get(),db_state.get());
    if(!v.valid())
        return nullptr;

    auto validators_percent=v->getValue("validator_count_percent");
    auto minstake=v->getValue("validator_min_stake");

    // Валидаторами (и лидерами) могут быть только включённые ноды со стейком >= minstake.
    // Иначе нода, созданная через node_create (нулевой стейк, никогда не запускается),
    // попадает в ротацию лидера — мёртвые слоты — и раздувает validator_count.
    for (auto& x : nnn)
        if (x.second->isEnabled() && x.second->get_full_stake() > 0
            && x.second->get_full_stake() >= minstake)
            live_nodes++;

    size_t validator_count=(live_nodes * validators_percent)/100;

    std::map<int64_t, std::map<NODE_id, REF_getter<const bc_node>>> result;

    for(auto &x: nnn)
    {
        if(!x.second->isEnabled())
            continue;
        // Нода без стейка — не валидатор: не должна попадать в ротацию лидера,
        // иначе созданная через node_create «фантомная» нода даёт мёртвые слоты.
        if(x.second->get_full_stake() <= 0)
            continue;
        if(x.second->get_full_stake() < minstake)
            continue;
        m->all_nodes_enabled.push_back(x.first);
        m->prev_stakes[x.second->getName()]=x.second->get_full_stake();
        std::string seed=x.first.container;
        if(prev_block.valid())
        {
            seed+=prev_block->blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1.container;
        }
        // else logNode("!if(prev_block.valid())");
        // seed+=std::to_string(round);
        auto h=blake2b_hash(seed);
        if(h.container.size()!=32) throw CommonError("if(h.container.size()!=32)");
        auto w=read_uint64((uint8_t*)h.container.data());
        auto fs=x.second->get_full_stake();
        if(fs)
            w/=fs;
        result[w].insert_or_assign(x.first,x.second);
    }
    for(auto& x:result)
    {
        for(auto& z:x.second)
        {
            m->position_of_node[z.second->getName()]=m->vne.size();
            m->vne.push_back(z.second->getElement());
        }
    }
    {
        std::vector<NodeElement> vne_bcast = m->vne;
        if (!unresponsive_nodes.empty()) {
            vne_bcast.clear();
            for (auto& n : m->vne)
                if (!unresponsive_nodes.count(n.name))
                    vne_bcast.push_back(n);
            if (vne_bcast.empty())
                vne_bcast = m->vne;   // все unresponsive — buildTree требует непустой
        }
        m->tree_all_nodes = buildTree(vne_bcast);
    }

    if(m->vne.empty())
        throw CommonError("vne is empty");
    if(validator_count>m->vne.size())
        validator_count=m->vne.size();      // не выходим за пределы vne
    std::vector<NodeElement> vne_committe;

    auto answered_min=v->getValue("answered_committee_min");

    // Попытка 1: строгий фильтр (answered > answered_min && stake >= minstake)
    for(size_t i=0; vne_committe.size()< validator_count && i< m->vne.size(); i++)
    {
        auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(),m->vne[i].name);
        if (!n.valid())
            continue;
        if (n->answered_rounds <= answered_min)
            continue;
        if(m->vne[i].stake_A>=minstake)
        {
            auto n=m->vne[i].name;
            vne_committe.push_back(m->vne[i]);
            m->committe_members.insert(n);
        }
    }

    // Попытка 2: без answered (только stake) — если строгий не набрался
    if (vne_committe.size() < validator_count)
    {
        // logNode("getMetaFull: strict filter gave %zu < %zu, relax answered",
                // vne_committe.size(), (size_t)validator_count);
        for(size_t i=0; vne_committe.size()< validator_count && i< m->vne.size(); i++)
        {
            if (m->committe_members.count(m->vne[i].name))
                continue;   // уже в комитете
            if(m->vne[i].stake_A>=minstake)
            {
                auto n=m->vne[i].name;
                vne_committe.push_back(m->vne[i]);
                m->committe_members.insert(n);
            }
        }
    }

    if (vne_committe.empty())
        throw CommonError("getMetaFull: no committee (even relaxed)");
    
    // std::string com;
    // for(auto &z : m->vne)
    // {
    //     com+=z.name.container+ " ";
    // }
    // logNode("vne %s",com.c_str());

    {
        std::vector<NodeElement> comm_bcast = vne_committe;
        if (!unresponsive_nodes.empty()) {
            comm_bcast.clear();
            for (auto& n : vne_committe)
                if (!unresponsive_nodes.count(n.name))
                    comm_bcast.push_back(n);
            if (comm_bcast.empty())
                comm_bcast = vne_committe;
        }
        m->tree_committe = buildTree(comm_bcast);
    }
    m->committee_vne=vne_committe;

    return m;
}

bool Node::Service::block_leader::is_ValidateBlockRSP_m_quorum(const THASH_id& h,  BlockMetaFull* mf)
{
    int64_t stakeVal = 0;
    for (auto &z : ValidateBlockRSP_m[h])
    {
        stakeVal += mf->getPrevStake(z->node_validator);;
    }
    int64_t fullstake=0;
    for(auto &z: mf->committe_members)
    {
        fullstake += mf->getPrevStake(z);

    }

    if ((stakeVal * 100) / fullstake > QUORUM )
        return true;
    return false;

}
bool Node::Service::block_leader::is_GetTransactionRSP_m_quorum(const THASH_id& h, Cellable*r, IDatabase* db)
{
    int64_t fullstakeVal = 0;
    auto ls=cells::getAllNodesConst(r, db);
    for (auto &z : ls)
    {
        if(z->isEnabled())
            fullstakeVal += z->get_full_stake();
    }
    int64_t stakeVal = 0;
    auto &mm=GetTxRSP_m[h];
    for (auto &z : mm)
    {
        auto n=cells::getNodeNoCreateConst(r, db,z.first);
        if(!n.valid())
            throw CommonError("if(!n.valid()) $555");
        stakeVal += n->get_full_stake();
    }
    if ((stakeVal * 100) / fullstakeVal > QUORUM )
        return true;
    return false;
}
bool Node::Service::block_leader::is_PingRSP_m_quorum(const THASH_id& h, Cellable* r, IDatabase* db)
{
    int64_t fullstakeVal = 0;
    auto ls=cells::getAllNodesConst(r, db);
    for (auto &z : ls)
    {
        if(z->isEnabled())
            fullstakeVal += z->get_full_stake();
    }
    int64_t stakeVal = 0;
    auto &mm=PingRSP_m[h];
    for (auto &z : mm)
    {
        auto n=cells::getNodeNoCreateConst(r, db,z.first);
        if(!n.valid())
            throw CommonError("if(!n.valid()) $p555");
        stakeVal += n->get_full_stake();
    }
    if ((stakeVal * 100) / fullstakeVal > QUORUM )
        return true;
    return false;
}

bool Node::Service::block_leader::is_BlockDiffValidateRSP_m_quorum(const THASH_id& h, Cellable*r, IDatabase* db)
{
    int64_t fullstakeVal = 0;
    auto ls=cells::getAllNodesConst(r, db);
    for (auto &z : ls)
    {
        if(z->isEnabled())
            fullstakeVal += z->get_full_stake();
    }
    int64_t stakeVal = 0;
    for (auto &z : BlockDiffValidateRSP_m[h])
    {
        auto n=cells::getNodeNoCreateConst(r, db,z->node_validator);
        if(!n.valid())
            throw CommonError("if(!n.valid()) $555");
        stakeVal += n->get_full_stake();
    }
    if ((stakeVal * 100) / fullstakeVal > QUORUM )
        return true;
    return false;
}

void Node::Service::logNode(const char *fmt, ...)
{

    int64_t ep=epoch_current();
    {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stdout, "%lf [Node] [%s] [%s] [%ld] ", double(iUtils->getNow().v) / 1000000., this_node_name.container.c_str(), prev_root_hash_Z().str().c_str(), ep);
        vfprintf(stdout, fmt, ap);
        fprintf(stdout, "\n");
        va_end(ap);
    }
    if(0) {
        va_list ap;
        va_start(ap, fmt);
        std::string pn=this_node_name.container+".log";
        FILE *f = fopen(pn.c_str(), "a");
        if (f)
        {
            fprintf(f, "%lf [Node] [%s] [%s] [%ld] ", double(iUtils->getNow().v) / 1000000., this_node_name.container.c_str(), prev_root_hash_Z().str().c_str(), ep);
            vfprintf(f, fmt, ap);
            fprintf(f, "\n");
            fclose(f);
        }
        va_end(ap);

        // fclose(f);
    }
}
