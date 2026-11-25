#include "nodeService.h"
#include "tools_mt.h"
#include <md_GetGranuleREQ.h>
#include <md_GetGranuleRSP.h>
#include <md_GetAddressInfoREQ.h>
#include <md_GetAddressInfoRSP.h>
#include <md_GetUserNonceREQ.h>
#include <md_GetUserNonceRSP.h>
#include <md_GetNodeInfoREQ.h>
#include <md_GetNodesREQ.h>
#include <md_GetNodesRSP.h>
#include <md_GetNodeInfoRSP.h>
#include <md_GetValuesREQ.h>
#include <md_GetValuesRSP.h>
#include <md_GetContractDataREQ.h>
#include <md_GetContractDataRSP.h>
#include <md_GetContractREQ.h>
#include <md_GetContractRSP.h>
#include <md_GetStateRSP.h>
#include <md_CallViewRSP.h>
#include <convert.h>
#include <md_CallViewREQ.h>
#include "jsi_pack.h"
#include <md_GetContractDataREQ.h>
#include "CallStack.h"
#include "i128.h"

// ============================================================
// SYNCING: пока sync_in_progress, Get*REQ → 503
// CallView / GetGranule / Scan — работают на stale root (не 503)
// ============================================================
static bool is_syncing_state_req(int p) {
    switch (p) {
    case msgid::GetUserNonceREQ:
    case msgid::GetNodeInfoREQ:
    case msgid::GetNodesREQ:
    case msgid::GetValuesREQ:
    case msgid::GetContractDataREQ:
    case msgid::GetContractREQ:
    case msgid::GetStateREQ:
    case msgid::GetAddressInfoREQ:
        return true;
    default:
        return false;
    }
}

static REF_getter<MsgData::Base> make_syncing_rsp(int p) {
    switch (p) {
    case msgid::GetUserNonceREQ:     return new MsgData::GetUserNonceRSP;
    case msgid::GetNodeInfoREQ:     return new MsgData::GetNodeInfoRSP;
    case msgid::GetNodesREQ:        return new MsgData::GetNodesRSP;
    case msgid::GetValuesREQ:       return new MsgData::GetValuesRSP;
    case msgid::GetContractDataREQ: return new MsgData::GetContractDataRSP;
    case msgid::GetContractREQ:     return new MsgData::GetContractRSP;
    case msgid::GetStateREQ:        return new MsgData::GetStateRSP;
    case msgid::GetAddressInfoREQ:  return new MsgData::GetAddressInfoRSP;
    default:                        return nullptr;
    }
}

bool Node::Service::ClientMsg(const bcEvent::ClientMsg *e)
{
    MUTEX_INSPECTOR;
    inBuffer in(e->msg);
// logNode("ClientMsg");
    auto p = in.get_PN();
    REF_getter<MsgData::Base> b = msgFactory.create(p);
    b->unpack(in);
    auto hash = b->getHash();
    // SYNCING: пока нода синкается — Get*REQ → 503 (CallView/GetGranule/Scan — не 503)
    if (sync_in_progress && is_syncing_state_req(p)) {
        auto rsp = make_syncing_rsp(p);
        if (rsp.valid()) {
            auto* rs = dynamic_cast<rsp_status*>(rsp.get());
            if (rs) rs->setError(msgcode::SYNCING, "node syncing");
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            return true;
        }
    }
    try {   // ← try/catch: root может быть устаревшим (Node записал блок, InvalidateRoot ещё не обработан) → hash not matched → throw
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
            int r = db_state->getGranule(pp->addr, &body);
            if (r == 0 && !body.empty()) {
                if (pp->hash.empty() || blake2b_hash(body).container == pp->hash) {
                    rsp->setOk();
                    rsp->stale = false;
                    rsp->body = body;
                }
            }

            // 2) stale_cache по hash (только full)
            if (!rsp->found() && !pp->hash.empty() && mode_ == Full) {
                std::string stale_body;
                if (get_stale(pp->hash, stale_body)) {
                    rsp->setOk();
                    rsp->stale = true;
                    rsp->body = stale_body;
                }
            }
            if (!rsp->found())
                rsp->setError(msgcode::NOT_FOUND, "granule not found");

            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetAddressInfoREQ: {
            // logNode("msgid::GetAddressInfoREQ:");
            auto pp = (MsgData::GetAddressInfoREQ*)b.get();
            REF_getter<MsgData::GetAddressInfoRSP> rsp = new MsgData::GetAddressInfoRSP;
            // rsp->addr = pp->addr;
            auto u = cells::getAddressStateNoCreateConst(root.get(), db_state.get(), pp->addr);   // normalize — в getPath
            if (u.valid()) {
                rsp->setOk();
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
            else {
                rsp->setError(msgcode::NOT_FOUND, "address not found");
            }
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetUserNonceREQ:
        {
            // logNode("msgid::GetUserNonceREQ");
            auto pp=(MsgData::GetUserNonceREQ*) b.get();
            // ▼▼▼ NoCreate — не создаём адрес (спам RPC)
            auto u = cells::getAddressStateNoCreateConst(root.get(),db_state.get(), pp->user_address);
#ifdef DEBUG
            logErr2("case msgid::GetUserNonceREQ: pp->user_address %s",pp->user_address.str().c_str());
#endif
            REF_getter<MsgData::GetUserNonceRSP> rsp = new MsgData::GetUserNonceRSP;
            if (u.valid()) {
#ifdef DEBUG
                logErr2("case msgid::GetUserNonceREQ: pp->user_address %s found",pp->user_address.str().c_str());
#endif
                rsp->nonce = u->nonce;
                rsp->balance = u->balance;
                rsp->setOk();
            } else {
#ifdef DEBUG
                logErr2("case msgid::GetUserNonceREQ: pp->user_address %s !found",pp->user_address.str().c_str());
#endif
                rsp->nonce = 0;
                rsp->balance = i128(0);
                rsp->setError(msgcode::NOT_FOUND,"404 not found");
            }
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            // ▲▲▲
        }
        break;
        case msgid::GetNodesREQ:
        {
            REF_getter<MsgData::GetNodesRSP> rsp = new MsgData::GetNodesRSP;
            auto nl = cells::getNodeListNoCreateConst(root.get(), db_state.get());
            if (nl.valid()) {
                rsp->setOk();
                for (auto& name : nl->getList()) {
                    auto node = cells::getNodeNoCreateConst(root.get(), db_state.get(), name);
                    if (node.valid()) rsp->nodes[name.container] = node->get_ip();
                }
            } else {
                rsp->setError(msgcode::NOT_FOUND, "node list not found");
            }
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetNodeInfoREQ:
        {
            auto pp=(MsgData::GetNodeInfoREQ*) b.get();
            REF_getter<MsgData::GetNodeInfoRSP> rsp = new MsgData::GetNodeInfoRSP;
            auto n = cells::getNodeNoCreateConst(root.get(),db_state.get(), pp->node_name);
            if (n.valid()) {
                rsp->setOk();
                rsp->name = n->getName();
                rsp->owner = n->get_owner();
                rsp->ed_pk = base16::encode(n->get_ed_pk());
                rsp->bls_pk = base16::encode(n->get_bls_pk().serialize());
                rsp->ip = n->get_ip();
                rsp->enabled = n->isEnabled();
                rsp->last_update = n->last_update.v;
                rsp->full_stake = n->get_full_stake();
                rsp->stakes = n->getStakes();
            }
            else {
                rsp->setError(msgcode::NOT_FOUND, "node not found");
            }
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetValuesREQ:
        {
            REF_getter<MsgData::GetValuesRSP> rsp = new MsgData::GetValuesRSP;
            auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
            if (v.valid()) {
                rsp->setOk();
                rsp->fees = v->values;
            } else {
                rsp->setError(msgcode::NOT_FOUND, "values not found");
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
            auto d = cells::getContractDataNoCreateConst(root.get(),db_state.get(), pp->contract, pp->key);
            if (d.valid()) {
                rsp->setOk();
                rsp->value = d->container;
            } else {
                rsp->setError(msgcode::NOT_FOUND, "contract data not found");
            }
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetContractREQ:
        {
            auto pp=(MsgData::GetContractREQ*) b.get();
            REF_getter<MsgData::GetContractRSP> rsp = new MsgData::GetContractRSP;
            rsp->contract = pp->contract;
            auto c = cells::getContractNoCreateConst(root.get(), db_state.get(), pp->contract);
            if (c.valid()) {
                rsp->setOk();
                rsp->owners = c->owners;
                rsp->src = c->src;
                auto act = cells::getActivityNoCreateConst(root.get(), db_state.get(), c->getDbId());
                if(!act.valid())
                    throw CommonError("if(!act.valid())");
                rsp->ttl = to_sec(act->ttl).v;   // наружу секунды   // ← activity.ttl (маркер), не bc_contract.ttl (0)
                rsp->update_time = act->_update_time.v;
            } else {
                rsp->setError(msgcode::NOT_FOUND, "contract not found");
            }
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }

        // в ClientMsg switch(p):
        case msgid::GetStateREQ:
        {
            REF_getter<MsgData::GetStateRSP> rsp = new MsgData::GetStateRSP;
            auto st = cells::getStateNoCreateConst(root.get(), db_state.get());
            if (st.valid()) {
                rsp->setOk();
                rsp->total_supply = st->total_supply.str();
                rsp->total_stake  = st->total_stake.str();
                rsp->account_count = st->account_count;
                rsp->node_count = st->node_count;
                rsp->contract_count = st->contract_count;
                rsp->tx_count = st->tx_count;
                rsp->block_height = st->block_height;
                rsp->genesis_time = st->genesis_time;
                rsp->chain_id = st->chain_id;
                rsp->root_hash = prev_root_hash_Z().str();
            } else {
                rsp->setError(msgcode::NOT_FOUND, "state not found");
            }
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::CallViewREQ:
        {
            auto pp = (MsgData::CallViewREQ*)b.get();
            REF_getter<MsgData::CallViewRSP> rsp = new MsgData::CallViewRSP;
            DBG(logErr2("[%s] Node CallViewREQ: contract=%s, view=%s, root=%s",
                        this_node_name.container.c_str(),
                        pp->contract.container.c_str(), pp->view.c_str(),
                        base16::encode(blake2b_hash(root->getBuffer_mx()).container).substr(0, 16).c_str()));            

            auto cc = contractHost->get_contract(root.get(), pp->contract);
            if (!cc.valid()) {
                rsp->setError(404,"contract not found");
                rsp->error = true;
                rsp->result_json = "contract not found: " + pp->contract.container;
            } else if (cc->views.find(pp->view) == cc->views.end()) {
                rsp->setError(404,"view not found");
                rsp->error = true;
                rsp->result_json = "view not found: " + pp->view;
            } else {
                rsp->setOk();
                JSIValue params = JSI_NULL;
                bool params_ok = true;
                try {
                    params = pp->params_json.empty()
                           ? JSI_NewObject(cc->ctx)
                           : unpack_jsi(cc->ctx, pp->params_json);
                } catch (const std::exception& e) {
                    params_ok = false;
                    rsp->error = true;
                    rsp->result_json = std::string("params: ") + e.what();
                }
                if (params_ok) {
                    CallStack cs;
                    CONTRACT_id caller;           // внешний вызов → пустой caller
                    TIME_mks now = iUtils->getNow();  // wall-clock, сэмпл один раз
                    TIME_mks chain_ts = prev_block.valid()
                        ? prev_block->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp : TIME_mks{};
                    JSIValue r = cc->call_query(root.get(), pp->view, params, &cs, contractHost.get(),
                        now, chain_ts);
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
                        rsp->result_json = granule_pack(cc->ctx, r, 0);
                        JSI_FreeValue(cc->ctx, r);
                    }
                }
                JSI_FreeValue(cc->ctx, params);
            }
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        default:
            logErr2("unhanddledf msg %s",msgName(p));
        }
    } catch (std::exception& ex) {
        // root устарел (Node записал новый блок, InvalidateRoot ещё не обработан) →
        // перечитать root + сбросить кэш + вернуть error (не timeout)
        DBG(logErr2("GrainReader::ClientMsg std::exception: %s (p=%s) → invalidate root",
                ex.what(), msgName(p)));
        root = getRoot(db_state.get(), load_last_block(db_state.get()));
        if (contractHost.valid()) contractHost->contract_cache.clear();

        // отправить ClientMsgReply с error (клиент не ждёт timeout)
        switch (p) {
        case msgid::CallViewREQ: {
            auto rsp = new MsgData::CallViewRSP;
            rsp->setError(404, ex.what());
            rsp->error = true;
            rsp->result_json = std::string("invalidate root: ") + ex.what();
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetGranuleREQ: {
            auto rsp = new MsgData::GetGranuleRSP;
            rsp->setError(msgcode::SYNCING, ex.what());
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetContractREQ: {
            auto rsp = new MsgData::GetContractRSP;
            rsp->setError(msgcode::SYNCING, ex.what());
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetContractDataREQ: {
            auto rsp = new MsgData::GetContractDataRSP;
            rsp->setError(msgcode::SYNCING, ex.what());
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetStateREQ: {
            auto rsp = new MsgData::GetStateRSP;
            rsp->setError(msgcode::SYNCING, ex.what());
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetValuesREQ: {
            auto rsp = new MsgData::GetValuesRSP;
            rsp->setOk();
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetNodesREQ: {
            auto rsp = new MsgData::GetNodesRSP;
            rsp->setError(msgcode::SYNCING, ex.what());
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetNodeInfoREQ: {
            auto rsp = new MsgData::GetNodeInfoRSP;
            rsp->setOk();
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetAddressInfoREQ: {
            auto rsp = new MsgData::GetAddressInfoRSP;
            rsp->setError(msgcode::SYNCING, ex.what());
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        case msgid::GetUserNonceREQ: {
            auto rsp = new MsgData::GetUserNonceRSP;
            rsp->setError(msgcode::SYNCING, ex.what());
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            break;
        }
        default:
            logErr2("GrainReader::ClientMsg: no error reply for %s", msgName(p));
            break;
        }
    }

    return true;
}
bool Node::Service::get_stale(const std::string& hash, std::string& out)
{
    auto it = stale_cache.find(hash);
    if (it == stale_cache.end()) return false;
    out = it->second.body;
    return true;
}

void Node::Service::cleanup_stale(int64_t now)
{
    for (auto it = stale_cache.begin(); it != stale_cache.end(); ) {
        if (it->second.ts + STALE_TTL_SEC < now) it = stale_cache.erase(it);
        else ++it;
    }
}

#include "CDatabaseLiteNode.h"
#include "NodeGranuleIO.h"
bool Node::Service::on_LiteSetUplinkREQ(const bcEvent::LiteSetUplinkREQ* e)
{
    MUTEX_INSPECTOR;
    if (mode_ != Lite) return true;
    // std::string db_path = "db/gr0";
    db_state = new CDatabaseLiteNode(db_name, e->addr);
    auto prev_root = load_last_block(db_state.get());
    root = getRoot(db_state.get(), prev_root);
    contractHost = new ContractHost();
    contractHost->granules = new NodeGranuleIO(db_state);
    contractHost->source = new NodeContractSourceIO(db_state);
    logErr2("GrainReader lite: node=%s", e->addr.c_str());
    return true;
}
