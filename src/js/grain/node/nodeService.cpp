#include "Events/System/Net/httpEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "base16.h"
#include "commonError.h"
#include "Events/Tools/telnetEvent.h"
// #include "bigint.h"
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
#include "init_root.h"
#include "nodeService.h"
#include "CDatabaseRocksdb.h"


#include "tr_exec.h"
#include "yyjson_to_quickjs.h"
#include "js_tools.h"
bool Node::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;



    last_activity_time=iUtils->getNow();

    SECURE sec;
    sec.use_ssl = false;
    for (auto &z : web_addr)
        sendEvent(ServiceEnum::HTTP, new httpEvent::DoListen(z, sec, this));

    db_state = new CDatabaseRocksdb(db_name);

    prev_block=load_last_block(db_state.get());
    // if (!root.valid())
    {
        db_state->root=getRoot(db_state.get(),prev_block);
        // prev_block=rrt.second;
    }
    init_root(db_state.get());

    if(prev_block.valid())
    {
            if(!verify_block_all(prev_block))
                prev_block=NULL;
                // throw CommonError("last_block not verified");

    }

    my_sk_bls.deserializebase16Str(getenv2(my_sk_bls_env_key));

    my_sk_ed = base16::decode(getenv2(my_sk_ed_env_key));
    logNode("ServiceInit nodename %s", this_node_name.container.c_str());
    sendEvent(ServiceEnum::BlockValidator, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_state, this));
    sendEvent(ServiceEnum::TxValidator, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_state, this));
    sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_state, this));
    sendEvent(ServiceEnum::GrainReader, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_state, this));
    for (auto &z : rpc_addr)
    {
        SECURE sec;
        sec.use_ssl = false;
        sendEvent(ServiceEnum::RPC, new rpcEvent::DoListen(z, sec));
    }
    sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(timers::TIMER_PERIODIC_CLOCK, NULL, NULL, 1., this));
    sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(timers::TIMER_REPORT_MEM, NULL, NULL, 30., this));

    std::string res;
    // int err = db_state->getGranule("#root_hash#", &res);
    // if (!err)
    // {
    //     // logNode("prev_root_hash_Z.container = res;");
    //     prev_root_hash_Z.container = res;
    // }

    // logNode("do_heart_beat in startService");
    // do_heart_beat();

    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^ds$", "show current element dump", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^go\\s+(.+)$", "go to child element", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^back$", "go to parent", ListenerBase::serviceId));


    // do_heart_beat();

    // REF_getter<MsgData::LcREQ> lr=new MsgData::LcREQ();
    // broadcast_MsgEvent(lr.get());
    // sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_LC_REQ_TIMEDOUT,NULL,NULL,1.,this));
    // state_Z=State::STATE_NORMAL;


    return true;
}

void Node::Service::collectTransactions()
{
    MUTEX_INSPECTOR;
    std::map<std::string /*user addr*/,
        std::map<uint64_t /*nonce*/, std::vector<REF_getter<MsgData::TX> > > > ordered;
    for (auto &z : transaction_pool_of_leader)
    {
        std::string &pk = z.second->pk_ed_bin;
        uint64_t nonce;
        auto err=z.second->getNonce(nonce);
        if(err)
            throw CommonError(err->c_str());
        ordered[pk][nonce].push_back(z.second);
    }
    transaction_pool_of_leader.clear();
    for (auto &x : ordered)
    {
        for (auto &y : x.second)
        {
            for (auto &z : y.second)
            {
                transaction_pool_of_leader.insert_or_assign(z->getHash(), z);
            }
        }
    }
}


void Node::Service::do_start_block()
{
    MUTEX_INSPECTOR;
    // logNode("@@ %s",__FUNCTION__);
    if (transaction_pool_of_leader.empty())
    {
        logNode("if (transaction_pool_of_leader.empty())");
        sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(timers::TIMER_RESTART_BLOCK, NULL, NULL, 0.5, this));
        return;
    }
    auto &li = l_blocks[prev_root_hash_Z()].leader_info;
    auto mf=getMetaFull(li.leader_cert_2->block_timestamp);
    {
        REF_getter<MsgData::ValidateBlockREQ> vb = new MsgData::ValidateBlockREQ();
        vb->heart_beat = li.leader_cert_2;

        auto &bt = l_blocks[prev_root_hash_Z()];
        collectTransactions();

        for (auto &z : transaction_pool_of_leader)
            vb->transaction_bodies.push_back(z.second);
        broadcast_MsgEvent_via_broadcaster(vb.get(), mf->tree_committe);
    }
}
bool Node::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
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
    case TIMER_BROADCAST_ACK_TIMEDOUT:
    {
        MUTEX_INSPECTOR;
        bcEvent::SendToChild *c = dynamic_cast<bcEvent::SendToChild *>(e->cookie.get());
        if (!c)
            throw CommonError("if(!c) 1222447");

        logErr2("TIMER_BROADCAST_ACK_TIMEDOUT %s", c->dstNodeName.container.c_str());

        make_broadcast_message_to_tree(c->dst_service,c->node_signer,c->node_start_timestamp,c->seqId2,c->payload_signature,  c->payload, c->bt, c->route);

        return true;
    }
    break;

    case timers::TIMER_SYNC_TIMEDOUT:
        logNode("FAILED SYNC, NODE STOPPED---------------------------------------------------------------");
    break;
    case timers::TIMER_VALIDATE_BLOCK_DELAY:
    {
        if(!db_state->sync_empty)
        {
            logNode("TIMER_VALIDATE_BLOCK_DELAY if(!db_state->sync_empty)");
            return true;
        }
        auto &li = l_blocks[prev_root_hash_Z()].leader_info;
        // auto &li = hbs.leader_info;
        do_start_block();
        logNode("do_start_block();");
        li.transaction_responders.clear();
        return true;
    }
    case timers::TIMER_RESTART_BLOCK:
    {
        if(!db_state->sync_empty)
        {
            logNode("TIMER_RESTART_BLOCK if(!db_state->sync_empty)");
            return true;
        }
        auto &li = l_blocks[prev_root_hash_Z()].leader_info;
        // auto &li = hbs.leader_info;
        li.request_for_transactions_sent = true;
        auto meta=getMetaFull(li.leader_cert_2->block_timestamp);
        do_request_for_transactions(li,meta);
        return true;
    }
    break;
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
        case bcEventEnum::SendToChild:
            return SendToChild(static_cast<const bcEvent::SendToChild *>(e.get()), false);
        case bcEventEnum::SendToChildAck:
            return SendToChildAck(static_cast<const bcEvent::SendToChildAck *>(e.get()), false);

        // case bcEventEnum::BroadcastMessage:
        //     return BroadcastMessage((const bcEvent::BroadcastMessage *)e.get());
        case bcEventEnum::GetGranulesREQ:
            return GetGranulesREQ((const bcEvent::GetGranulesREQ *)e.get());
        case bcEventEnum::GetGranulesRSP:
            return GetGranulesRSP((const bcEvent::GetGranulesRSP *)e.get());
        case bcEventEnum::NodeMsgREQ:
            return NodeMsgREQ((const bcEvent::NodeMsgREQ *)e.get());
        case bcEventEnum::NodeMsgRSP:
            return NodeMsgRSP((const bcEvent::NodeMsgRSP *)e.get());
        case bcEventEnum::PutTransactionREQ:
            return PutTransactionREQ((const bcEvent::PutTransactionREQ *)e.get());
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
            case bcEventEnum::SendToChild:
                return SendToChild(static_cast<const bcEvent::SendToChild *>(ev->e.get()), true);
            case bcEventEnum::SendToChildAck:
                return SendToChildAck(static_cast<const bcEvent::SendToChildAck *>(ev->e.get()), true);

            // case bcEventEnum::BroadcastMessage:
            //     return BroadcastMessage((const bcEvent::BroadcastMessage *)ev->e.get());
            case bcEventEnum::GetGranulesREQ:
                return GetGranulesREQ((const bcEvent::GetGranulesREQ *)ev->e.get());
            case bcEventEnum::GetGranulesRSP:
                return GetGranulesRSP((const bcEvent::GetGranulesRSP *)ev->e.get());
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
            case bcEventEnum::SendToChild:
                return SendToChild(static_cast<const bcEvent::SendToChild *>(ev->e.get()), true);
            case bcEventEnum::SendToChildAck:
                return SendToChildAck(static_cast<const bcEvent::SendToChildAck *>(ev->e.get()), true);
            // case bcEventEnum::BroadcastMessage:
            //     return BroadcastMessage((const bcEvent::BroadcastMessage *)ev->e.get());
            case bcEventEnum::GetGranulesREQ:
                return GetGranulesREQ((const bcEvent::GetGranulesREQ *)ev->e.get());
            case bcEventEnum::GetGranulesRSP:
                return GetGranulesRSP((const bcEvent::GetGranulesRSP *)ev->e.get());
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
    JS_FreeRuntime(contract_runtime);

}

Node::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins),
      iInstance(ins)
{
    MUTEX_INSPECTOR;
    // rocksdb_path = ins->getConfig()->get_string("rockdb_path", "/db/r1", "Path to access to rocksdb");
    // sqlite_pn = ins->getConfig()->get_string("sqlite_pn", "/db/1", "Pathname to access to sqlite");
    rpc_addr = ins->getConfig()->get_tcpaddr("rpc_addr", "127.0.0.1:2345", "rpc address(es) of node ex: ip:port,ip2:port2");
    web_addr = ins->getConfig()->get_tcpaddr("web_addr", "127.0.0.1:2347", "web address(es) of node ex: ip:port,ip2:port2");
    my_sk_bls_env_key = ins->getConfig()->get_string("my_sk_bls_env_key", "sk_bls_env_key", "env key of bls key");
    my_sk_ed_env_key = ins->getConfig()->get_string("my_sk_ed_env_key", "sk_ed_env_key", "env key of ed key");
    this_node_name.container = ins->getConfig()->get_string("this_node_name", "n0", "registered name of node");

    db_name=ins->getConfig()->get_string2("db_name", "grain", "db name");
    contract_runtime=JS_NewRuntime();
    node_start_timestamp=iUtils->getNow();
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
    }
    else
    {
        iUtils->registerService(ServiceEnum::Node, Node::Service::construct, "Node");
        regEvents_nodeService();
    }
    XPASS;
}

bool Node::Service::RequestIncoming(const httpEvent::RequestIncoming *e)
{
    MUTEX_INSPECTOR;
    logNode("RequestIncoming %s", e->req->url.c_str());
    HTTP::Response r(e->req);
    auto uri = (std::string)e->req->url;
    auto da = iUtils->splitString("/", uri);
    // auto buf = c->dump();
    // r.make_response("<pre>" + j.dump(2) + "</pre>");
    return true;
}

void Node::Service::do_request_for_transactions( heart_beat_node_info& li, const REF_getter<BlockMetaFull>& meta)
{
    MUTEX_INSPECTOR;

    REF_getter<MsgData::GetTransactionREQ> rt = new MsgData::GetTransactionREQ();
    if(!li.leader_cert_2.valid())
    {
        throw CommonError("if(!li.leader_cert_2.valid())");
    }
    rt->lc = li.leader_cert_2;
    li.request_for_transactions_time = iUtils->getNow();

    broadcast_MsgEvent_via_broadcaster(rt.get(),meta->tree_all_nodes);
}

// #include "sql"
THASH_id Node::Service::execute_block(b_params &b,  const REF_getter<MsgData::HeartBeatREQ> &lc)
{
    MUTEX_INSPECTOR;
    // M_LOCK(root->state_mutex);
    // outBuffer o;
    for (int ti = 0; ti < b.validateBlockREQ->transaction_bodies.size(); ti++)
    {
        MUTEX_INSPECTOR;
        std::optional<std::string> t_err;
        auto tt=b.validateBlockREQ->transaction_bodies[ti];
        auto tx_hash=tt->getHash();
        auto &pk_bin=tt->pk_ed_bin;
        ADDRESS_id senderAddress;
        senderAddress.addr=blake2b_hash(pk_bin).container;
        auto &tj = tt->tx_body;
        if (!tt->verify())
        {
            t_err = "verify failed @12";
            logNode("verify failed @12");
        }
        if (!t_err)
        {
            MUTEX_INSPECTOR;
            auto u = b.db->getAddressStateOrCreate(senderAddress,NULL);
            if (!u.valid())
            {
                t_err = "sender invalid";
                logNode("sender invalid");
            }
            if (!t_err)
            {
                uint64_t nonce;
                auto err=tt->getNonce(nonce);
                if(err) throw CommonError(*err);
                if (u->getNonce() != nonce)
                {
                    logNode("invalid nonce, expected %lld got %lld", u->getNonce(), nonce);
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
                    t_err=execute_transaction(tt->getHash(),  b, senderAddress, tt, epoch_current());
                    if(!t_err)
                    {
                        u->incNonce();
                    }
                    u->setDirty(NULL);

                }
            }
        }
        if (!t_err)
            b.emit_tx(tx_hash, "result", R"({"success":true})");
        else
            b.emit_tx(tx_hash, "error", R"({"error":"%s"})", t_err->c_str());    
    }

    auto rh=proceed_merkle_on_transaction_pool_hashers(db_state->root);
    calc_fee_rewards_nodes(b, lc);

    rh=proceed_merkle_on_transaction_pool_hashers(db_state->root);
    return rh;
}
void Node::Service::calc_fee_rewards_nodes(b_params &b, const REF_getter<MsgData::HeartBeatREQ> &lc)
{
    MUTEX_INSPECTOR;

    double total_staked=0;
    auto nn=db_state->getAllNodes();
    auto local_prev_block=prev_block;
    std::set<NODE_id> ns;
    if(local_prev_block.valid())
    {
        for(auto& z:local_prev_block->blockAcceptedREQ->node_validators)
        {
            ns.insert(z);
            auto n=db_state->getNodeNoCreate(z);
            if(!n.valid())
                throw CommonError("if(!n.valid())");

            total_staked+=n->get_full_stake();
        }
        for(auto& z:local_prev_block->blockAcceptedREQ->node_validators)
        {
            auto n=db_state->getNodeNoCreate(z);
            if(!n.valid())
                throw CommonError("if(!n.valid())");

            auto portion=n->get_full_stake()*b.node_rewards/total_staked;
            auto u = db_state->getAddressStateOrCreate(n->get_owner(),NULL);
            {
                M_LOCK(u->parent->mx);
                u->balance+=portion;
            }
            u->setDirty(NULL);
            b.emit_block("reward",R"({"node":"%s","fee":"%s"})",z.container.c_str(),std::to_string(portion).c_str());
        }
        
    }


    b.emit_block("total_fee",R"({"fee":"%s"})",std::to_string(b.node_rewards).c_str());
}

THASH_id Node::Service::proceed_merkle_on_transaction_pool_hashers(const REF_getter<Cellable> &r)
{
    MUTEX_INSPECTOR;
    r->calc_tree_hash(db_to_save_Z);
    // r->calcers_Z.clear();

    std::string root_buf;
    {
        M_LOCK(r->mx);
        root_buf = r->getBuffer_mx();
    }
    auto root_hash = blake2b_hash(root_buf);
    db_to_save_Z.add("", root_buf);
    // db_to_save_Z.add("#root_hash#", root_hash.container);
    THASH_id ret;
    ret.container = root_hash.container;
    return ret;
}
#include <stdlib.h>
bool Node::Service::isNodeGreater(const NODE_id &nodeLeft, const NODE_id &nodeRight, const REF_getter<BlockMetaFull>& m)
{
    MUTEX_INSPECTOR;
    auto itL=m->position_of_node.find(nodeLeft);
    if(itL == m->position_of_node.end())
        throw CommonError("if(itL == m->position_of_node.end())");
    auto itR=m->position_of_node.find(nodeRight);
    if(itR == m->position_of_node.end())
        throw CommonError("if(itR == m->position_of_node.end())");
        /// TODO
    // if(prev_root_hash_Z().container.size())
        return itL->second < itR->second;
    return nodeLeft.container<nodeRight.container;
}
bool Node::Service::verify_block_committee(const REF_getter<MsgData::BlockAcceptedREQ> &lc)
{
    /// проверка сертификата лидера
    if(!lc.valid())
        return false;
    {
        MUTEX_INSPECTOR;
        auto mf=getMetaFull(lc->blockInfo->heart_beat->block_timestamp);
        std::vector<blst_cpp::PublicKey> agg_pk;

        uint64_t stake=0;
        for (auto &z : lc->node_validators)
        {
            auto n = db_state->getNodeNoCreateConst(z);
            agg_pk.push_back(n->get_bls_pk());
            stake += mf->getPrevStake(z);
        }
        uint64_t fullstake=0;
        for(auto& z: mf->committe_members)
        {
            fullstake=mf->getPrevStake(z);
        }
        if ((stake * 100) / fullstake < QUORUM)
        {
            logNode("this quorum %lld",(stake * 100) / fullstake);
            logErr2("verify lc quorum failed %lld %lld",stake,fullstake);
            return false;
        }
        if (!lc->agg_sig.verify(agg_pk, lc->blockInfo->getHash().container))
        {
            logErr2("verify lc - signature invalid");
            ;
            return false;
        }
    }

    return true;
}
bool Node::Service::verify_block_all(const REF_getter<MsgData::BlockAccepted2REQ> &lc)
{
    /// проверка сертификата лидера
    if(!lc.valid())
        return false;
    {
        MUTEX_INSPECTOR;
        // auto mf=getMetaFull(lc->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp);
        std::vector<blst_cpp::PublicKey> agg_pk;

        uint64_t stake=0;
        for (auto &z : lc->node_diff_validators)
        {
            auto n = db_state->getNodeNoCreateConst(z);
            agg_pk.push_back(n->get_bls_pk());
            stake += n->get_full_stake();
        }
        uint64_t full_stake=0;
        auto ls=db_state->getAllNodes();
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
            logErr2("verify lc quorum failed %lld %lld",stake,full_stake);
            return false;
        }
        if (!lc->agg_diff_sig.verify(agg_pk, lc->blockAcceptedREQ->blockInfo->getHash().container))
        {
            logErr2("verify lc - sign invalid");
            ;
            return false;
        }
    }

    return true;
}

bool Node::Service::PutTransactionREQ(const bcEvent::PutTransactionREQ *e)
{
    MUTEX_INSPECTOR;
    logNode("@@ %s",__FUNCTION__);
    auto h=e->tx->getHash();
    transaction_pool_of_leader.insert_or_assign(h,e->tx);
    logNode("stage_is_working %ld",stage_is_working);
    if(iUtils->getNow()-stage_is_working> STAGE_IS_WORKING_TIMEOUT* _1sec)
    {
        stage_is_working=iUtils->getNow();
            
        do_heart_beat(time(NULL));
    }
    else {
        logNode("no heart beat timediff %ld",iUtils->getNow()-stage_is_working);
    }
    return true;
}

bool Node::Service::NodeMsgREQ(const bcEvent::NodeMsgREQ *m)
{
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
    auto n = db_state->getNodeNoCreate(m->node_signer);
    if(!n.valid())
        return true;
    if (!verify_ed_pk(n->get_ed_pk(), m->signature, blake2b_hash(m->msg_payload)))
    {
        logNode("verify failed 11");
        return true;
    }
    inBuffer in(m->msg_payload);
    auto id = in.get_PN();

    REF_getter<MsgData::Base> msg = msgFactory.create(id);
    msg->unpack(in);

    switch (msg->type)
    {
    case msgid::GetTransactionREQ:
        return GetTransactionREQ(static_cast<const MsgData::GetTransactionREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::ValidateBlockREQ:
        last_activity_time=iUtils->getNow();
        return ValidateBlockREQ(static_cast<const MsgData::ValidateBlockREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::BlockAccepted2REQ:
        last_activity_time=iUtils->getNow();
        return BlockDBStore(static_cast<const MsgData::BlockDBStore *>(msg.get()), m->node_signer, m->route);
    case msgid::ConfirmLeaderREQ:
        return ConfirmLeaderREQ(static_cast<const MsgData::ConfirmLeaderREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::DelayNotificationREQ:
        return DelayNotificationREQ(static_cast<const MsgData::DelayNotificationREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::BlockDiffValidateREQ:
        return BlockDiffValidateREQ(static_cast<const MsgData::BlockDiffValidateREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::BlockDBStore:
        return BlockDBStore(static_cast<const MsgData::BlockDBStore *>(msg.get()), m->node_signer, m->route);

    default:
        throw CommonError("unjandled3 MsgData %s", msgName(msg->type));
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

    auto n = db_state->getNodeNoCreate(m->node_signer);
    if(!n.valid())
        throw CommonError("if(!n.valid())");
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
    case msgid::HeartBeatRSP:
        return HeartBeatRSP(static_cast<const MsgData::HeartBeatRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::ConfirmLeaderRSP:
        return ConfirmLeaderRSP(static_cast<const MsgData::ConfirmLeaderRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::GetTransactionRSP:
        return GetTransactionRSP(static_cast<const MsgData::GetTransactionRSP *>(ee.get()), m->node_signer, m->route);
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
std::optional<std::string> Node::Service::execute_tx_commands(b_params &b, t_params& t, 
      yyjson_val * j_tx)
{
    MUTEX_INSPECTOR;
    if(!yyjson_is_arr(j_tx))
        return "command list must be json array";
        // throw CommonError("if(!yyjson_is_arr(root))");
    yyjson_arr_iter iter;
    yyjson_arr_iter_init(j_tx, &iter);    
    // if(root.isArray())
    uint32_t index = 0;
    yyjson_val* item;
    {
        MUTEX_INSPECTOR;
        if(t.gasUsed>t.gasLimit) return "gas exceeds limit";
        while ((item = yyjson_arr_iter_next(&iter)))
        {
            MUTEX_INSPECTOR;
            bool err=false;
            yyjson_val* contract = yyjson_obj_get(item, "contract");
            yyjson_val* method = yyjson_obj_get(item, "method");
            yyjson_val* params = yyjson_obj_get(item, "params");
            if(contract==NULL)
                return "'contract' field required";
            if(method==NULL)
                return "'method' field required";
            if(params==NULL)
                return "'params' field required";
            if(!yyjson_is_str(contract))
                return "'contract' must be string type";
            if(!yyjson_is_str(method))
                return "'method' must be string type";
            if(!yyjson_is_obj(params))
                return "'params' must be object type";
            std::string contract_str=yyjson_get_str(contract);
            std::string method_str=yyjson_get_str(method);
            if (!err && contract_str == "root")
            {
                MUTEX_INSPECTOR;
                std::optional<std::string> err;
                auto meth=method_str;
                // logErr2("method %s",meth.c_str());
                if (meth == "mint")
                    err = TR::execute_mint(params, b, t,  index);
                else if (meth == "transfer")
                    err = TR::execute_transfer(params, b,t,  index);
                else if (meth == "node_create")
                    err = TR::execute_node_create(params,b, t,  index);
                else if (meth == "node_update")
                    err = TR::execute_node_update(params, b,t,  index);
                else if (meth == "node_stake")
                    err = TR::execute_node_stake(params, b,t,  index);
                else if (meth == "node_unstake")
                    err = TR::execute_unstake_node(params, b,t,  index);
                else if (meth == "node_enable")
                    err = TR::execute_node_enable(params, b,t,  index);
                else if (meth == "node_disable")
                    err = TR::execute_node_disable(params, b,t,  index);
                else if (meth == "contract_deploy")
                    err = TR::execute_contract_deploy(params,b, t,  index);
                else if (meth == "contract_update")
                    err = TR::execute_contract_update(params, b,t,  index);
                else
                {
                    MUTEX_INSPECTOR;
                    return "unhandled method '"+method_str+"' for root contract";
                }
                if (err)
                {
                    MUTEX_INSPECTOR;
                    return err;
                }

            }
            else if(!err)
            {
                MUTEX_INSPECTOR;
                CONTRACT_id c;
                c.container=contract_str;
                auto m=method;
                
            }

        }
    }
    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_transaction(const THASH_id &tx_id, b_params &b, const ADDRESS_id &senderAddress, 
    const REF_getter<MsgData::TX> &tx, uint64_t epoch)
{
    MUTEX_INSPECTOR;
    // yyjson::Document doc(tx_cmds);
    uint64_t gasLimit=0;
    uint64_t gasPrice=0;
    uint64_t value=0;
    
    yyjson_val *jroot=yyjson_doc_get_root(tx->doc);

    yyjson_val * j_tx = yyjson_obj_get(jroot,"tx");
    if(!j_tx)
        throw CommonError("if(!j_tx)");
    auto err=yy_get_uint64_t(jroot,"value",value);
    if(!err)
        err=yy_get_uint64_t(jroot,"gasLimit",gasLimit);
    if(!err)
        err=yy_get_uint64_t(jroot,"gasPrice",gasPrice);
    if(err)
    {
        b.emit_tx(tx_id,"error",R"({"error":"%s"})",err->c_str());
        return err;
    }    
    Rollback roll;
    t_params t(db_state);
    t.senderAddress=senderAddress;
    t.tx=tx;
    t.epoch=epoch;
    t.tx_id=tx_id;
    t.roll=&roll;
    t.value=value;
    t.gasLimit=gasLimit;
    auto uu=db_state->getAddressStateNoCreate(senderAddress);
    if(!uu.valid())
    throw CommonError("if(!uu.valid())");
    {
        M_LOCK(uu->parent->mx);
        if(uu->balance < gasLimit*gasPrice+value)
            return "not enough funds to reserve gasLimit*gasPrice+value";
    }
    /// сбрасываем все изменения состояния перед транзакцией
    // _db_to_save db_dump0;
    db_state->root->calc_tree_hash(db_to_save_Z);

    err=execute_tx_commands(b,t,j_tx);
    if(err)
    {
        logNode("error:%s",err->c_str());
        b.emit_tx(t.tx_id,"error",R"({"error":"%s"})",err->c_str());
        t.gasUsed+=t.roll->size();
        t.rollback();
        auto gu=t.gasUsed;
        if(gu>gasLimit)
            gu=gasLimit;

        // auto u=db_state->getAddressStateOrCreate(t.senderAddress,NULL);
        M_LOCK(uu->parent->mx);
        uu->balance-=gu*gasPrice;
        b.node_rewards+=gu*gasPrice;
        
        return err;

    }
    if(t.value<0)
    {
        t.gasUsed+=t.roll->size();
        t.rollback();
        b.emit_tx(t.tx_id,"error",R"({"error":"value exceeds limit"})");
        // auto u=db_state->getAddressStateOrCreate(t.senderAddress,NULL);
        M_LOCK(uu->parent->mx);
        uu->balance-=t.gasUsed*gasPrice;
        b.node_rewards+=t.gasUsed*gasPrice;
        return "value exceeds limit";
    }
    if(t.gasUsed>gasLimit)
    {
        t.rollback();
        b.emit_tx(t.tx_id,"error",R"({"error":"gas exceeds limit"})");
        // auto u=db_state->getAddressStateOrCreate(t.senderAddress,NULL);
        M_LOCK(uu->parent->mx);
        uu->balance-=gasLimit*gasPrice;
        b.node_rewards+=gasLimit*gasPrice;
        return "gas exceeds limit";
    }

    _db_to_save db_dump;
    db_state->root->calc_tree_hash(db_dump);
    size_t sz=db_dump.size();
    t.gasUsed+=sz;
    if(t.gasUsed>gasLimit)
    {

        t.rollback();
        b.emit_tx(t.tx_id,"error",R"({"error":"gas exceeds limit"})");
        // auto u=db_state->getAddressStateNoCreate(t.senderAddress);
        // if(!u.valid())
        //     throw CommonError("if(!u.valid()) AAA");

        M_LOCK(uu->parent->mx);
        uu->balance-=gasLimit*gasPrice;
        b.node_rewards+=gasLimit*gasPrice;
        return "gas exceeds limit";
    }
    // OK
    // auto u=db_state->getAddressStateOrCreate(t.senderAddress,NULL);
    {
        M_LOCK(uu->parent->mx);
        uu->balance-=t.gasUsed*gasPrice+value-t.value;
    }
    // for(auto& z:t.node_enables)
    // {
    //     b._node_enables.insert_or_assign(z.first,z.second);
    // }
    // for(auto& z: t.node_stake_changes)
    // {
    //     for(auto& y: z.second)
    //     {
    //         b._node_stake_changes[z.first][y.first]+=y.second;    
    //     }
        
    // }

    db_state->root->calc_tree_hash(db_dump);
    db_to_save_Z.add(db_dump);
    
    return std::nullopt;
}
std::optional<std::string> Node::Service::execute_contract(const CONTRACT_id& ct, const std::string & method, yyjson_val* params)
{
    auto it=contracts.find(ct);
    if(it==contracts.end())
    {
        auto err=load_contract(ct);
        if(err)
            return err;
        it==contracts.find(ct);
        if(it==contracts.end())
            throw CommonError("if(it--contracts.end())");
    }
    JSScope<10, 10> scope(it->second->ctx);
    YYJsonToQuickJS converter(it->second->ctx);
    JSValue jspars=converter.convert(params);
    scope.addValue(jspars);
    auto mi=it->second->methods.find(method);
    if(mi==it->second->methods.end())
        return "method not found";

    return std::nullopt;
}
#include "jsscope.h"
#include "js_tools.h"
std::optional<std::string> Node::Service::load_contract(const CONTRACT_id& contract)
{
    auto c=db_state->getContract(contract);
    REF_getter<contract_rt> ct=new contract_rt();
    contracts.insert_or_assign(contract,ct);
    ct->ctx=JS_NewContext(contract_runtime);
    {
        M_LOCK(c->parent->mx);
        ct->src=c->src;
        ct->owner=c->owner;
    }
    
    JSScope<10, 10> scope(ct->ctx);
    JSValue module1 = JS_Eval(ct->ctx, ct->src.data(), ct->src.size(), "<module>", JS_EVAL_TYPE_MODULE | JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);

    std::string err;
    if (qjs::CheckAndGetException(ct->ctx, module1, "loadModule",err))
        return err;

    JSValue ret = JS_EvalFunction(ct->ctx, module1);
    scope.addValue(ret);
    if (qjs::CheckAndGetException(ct->ctx, ret, "loadModule",err))
        return err;

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

REF_getter<BlockMetaFull> Node::Service::getMetaFull(time_t ti_)
{
    MUTEX_INSPECTOR;
    auto b=prev_root_hash_Z();
    auto t_win=ti_ / HB_TIME_WINDOW;
    auto it=block_meta_full.find(b);

    if(it!=block_meta_full.end())
    {
        auto ii=it->second.find(t_win);
        if(ii!=it->second.end())
        {
            if(ii->second.valid())
                return ii->second;

        }
    }
    REF_getter<BlockMetaFull> m=new BlockMetaFull();
    block_meta_full[b][t_win]=m;
    // auto nodeList=db_state->getNodeListNoCreateConst();
    // m->full_broadcast=nn->getList();
    auto an=db_state->getAllNodes();
    int live_nodes=0;
    for(auto& z: an)
    {
        if(z->isEnabled())
        {
            live_nodes++;
        }
    }
            

    auto v=db_state->getValuesNoCreateConst();
    
    auto validators_percent=v->getGas("validator_count_percent");

    size_t validator_count=(live_nodes * validators_percent)/100;

    std::vector<NodeElement> vne;
    std::map<uint64_t, std::map<NODE_id, REF_getter<bc_node>>> result;

    for(auto &x: an)
    {
        if(!x->isEnabled())
            continue;
        m->all_nodes_enabled.push_back(x->getName());
        m->prev_stakes[x->getName()]=x->get_full_stake();
        std::string seed=x->getName().container;
        if(prev_block.valid())
        {
            seed+=prev_block->blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1.container;
        }
        seed+=std::to_string(t_win);
        auto h=blake2b_hash(seed);
        if(h.container.size()!=32) throw CommonError("if(h.container.size()!=32)");
        auto w=read_uint64((uint8_t*)h.container.data());
        auto fs=x->get_full_stake();
        if(fs)
            w/=x->get_full_stake();
        result[w].insert_or_assign(x->getName(),x);
    }
    for(auto& x:result)
    {
        for(auto& z:x.second)
        {
            m->position_of_node[z.second->getName()]=vne.size();
            vne.push_back(z.second->getElement());
        }
    }
    m->tree_all_nodes=buildTree(vne);
    // logNode("tree_all_nodes %s",m->tree_all_nodes.jdump().dump(2).c_str());

    if(validator_count>=vne.size())
        throw CommonError("if(validator_count>=vne.size())");
    std::vector<NodeElement> vne_committe;

    // std::string vne_list;
    // for(auto& z: vne)
    // {
    //     vne_list+=z.name.container+ " ";
    // }
    // logNode("vne %s",vne_list.c_str());
    // std::string comlist;
    // logNode("validator_count %d",validator_count);
    for(size_t i=0;i<validator_count;i++)
    {
        auto n=vne[i].name;
        // comlist+=n.container+" ";
        // logNode("commitee %s",n.container.c_str());
        vne_committe.push_back(vne[i]);
        // m->committe_full_stake+=vne[i].stake_A;
        m->committe_members.insert(n);
    }
    // logNode("committee %s",comlist.c_str());
    m->tree_committe=buildTree(vne_committe);

    return m;
}
void Node::Service::logNode(const char *fmt, ...)
{

    uint64_t ep=epoch_current();
    {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stdout, "%lf [Node] [%s] [%s] [%ld] ", double(iUtils->getNow()) / 1000000., this_node_name.container.c_str(), prev_root_hash_Z().str().c_str(), ep);
        vfprintf(stdout, fmt, ap);
        fprintf(stdout, "\n");
        va_end(ap);
    }
    if(0){
        va_list ap;
        va_start(ap, fmt);
        std::string pn=this_node_name.container+".log";
        FILE *f = fopen(pn.c_str(), "a");
        if (f)
        {
            fprintf(f, "%lf [Node] [%s] [%s] [%ld] ", double(iUtils->getNow()) / 1000000., this_node_name.container.c_str(), prev_root_hash_Z().str().c_str(), ep);
            vfprintf(f, fmt, ap);
            fprintf(f, "\n");
            fclose(f);
        }
        va_end(ap);
        
        // fclose(f);
    }
}
