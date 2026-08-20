#include "Events/System/Net/httpEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "base16.h"
#include "commonError.h"
#include "Events/Tools/telnetEvent.h"
#include "bigint.h"
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
#include "execute_transaction.h"
#include "root_contract.h"
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
#include "CDatabase.h"


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

    auto db=getDB();
    db_state = new CDatabase(db, db_name);

    if (!root.valid())
    {
        auto rrt = getRoot(db_state.get());
        root=rrt.first;
        prev_block=rrt.second;
        // if(rrt.second.valid())
        // {
        //     if(!verify_block(rrt.second))
        //     throw CommonError("last_block not verified");

        //     prev_block=rrt.second;
        // }
    }
    init_root(root,db_state.get());

    if(prev_block.valid())
    {
            if(!verify_block(prev_block))
                throw CommonError("last_block not verified");

    }

    my_sk_bls.deserializebase16Str(getenv2(my_sk_bls_env_key));

    my_sk_ed = base16::decode(getenv2(my_sk_ed_env_key));
    logNode("ServiceInit nodename %s", this_node_name.container.c_str());
    sendEvent(ServiceEnum::BlockValidator, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_name, root, this));
    sendEvent(ServiceEnum::TxValidator, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_name, root, this));
    sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_name, root, this));
    sendEvent(ServiceEnum::GrainReader, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_name, root, this));
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

    logNode("do_heart_beat in startService");
    // do_heart_beat();

    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^ds$", "show current element dump", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^go\\s+(.+)$", "go to child element", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^back$", "go to parent", ListenerBase::serviceId));


    // do_heart_beat();

    // REF_getter<MsgData::LcREQ> lr=new MsgData::LcREQ();
    // broadcast_MsgEvent(lr.get());
    // sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_LC_REQ_TIMEDOUT,NULL,NULL,1.,this));
    state_Z=State::STATE_NORMAL;


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
    // auto &li = hbs.leader_info;
    {
        {
            // make_leader_certificate();
            REF_getter<MsgData::ValidateBlockREQ> b = new MsgData::ValidateBlockREQ();
            // msg::block_request b;
            b->heart_beat = li.leader_cert_2;

            // std::set<std::string> nnn;
            // for(auto& z:b->heart_beat->nodes)
            // {
            //     nnn.insert(z.container);
            // }
            // logNode("LC nodes %s",iUtils->join(" ",nnn).c_str());

            auto &bt = l_blocks[prev_root_hash_Z()];
            // logNode("before collectTransactions sz %d", transaction_pool_of_leader.size());
            collectTransactions();
            // logNode("AFTER collectTransactions sz %d", transaction_pool_of_leader.size());

            for (auto &z : transaction_pool_of_leader)
                b->transaction_bodies.push_back(z.second);
            // logNode("broadcast ValidateBlockREQ");
            broadcast_MsgEvent(b.get());
        }
    }
}
void Node::Service::broadcast_MsgEvent(const REF_getter<MsgData::Base>& b)
{
    std::string msg;
    // logErr2("b get %p",b.get());
    if(b.valid())
        msg=b->getBuffer();
    // logErr2("KALL 1");
    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    sendEvent(ServiceEnum::BroadcasterTree,
              new bcEvent::BroadcastMessage(ServiceEnum::Node,
                                            this_node_name, node_start_timestamp, seqId2++, signature,msg, ListenerBase::serviceId));

}
bool Node::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    if(e->tid==timers::TIMER_REPORT_MEM)
    {
        report_mem();
    }
    return true;
}
void Node::Service::report_mem()
{
    size_t sz=0;
    for(auto&z: c_blocks)
    {
        sz+=z.first.container.size();
        sz+=z.second.size();
    }
    for(auto& z: l_blocks)
    {
        sz+=z.first.container.size();
        sz+=z.second.size();
    }
    for(auto &z : cli_leader_info)
    {
        sz+=z.first.container.size();
        sz+=z.second.size();
    }
    for(auto &z : syncs)
    {
        sz+=z.first.container.size();
        sz+=z.second.size();
    }
    for(auto &z : filter_NodeMsgREQ)
    {
        sz+=z.first.container.size();
        for(auto x: z.second)
        {
            sz+=sizeof(x.first);
            for(auto y: x.second)
            {
                sz+=sizeof(y);
            }
        }

        // sz+=z.second.size();
    }
    for(auto& z: transaction_pool_of_leader)
    {
        sz+=z.first.container.size();
        sz+=z.second->size();
    }
    logNode("REPORT_MEM NodeService %ld",sz);
        //     std::map<BLOCK_id, block_client> c_blocks;
        // std::map<BLOCK_id,block_leader> l_blocks;
        // std::map<BLOCK_id, client_leader_info> cli_leader_info;
        // std::map<BLOCK_id,_sync> syncs;
        // std::map<NODE_id,std::map<int64_t,std::set<int64_t> > > filter_NodeMsgREQ;
        // std::map<CONTRACT_id, REF_getter<contract_rt> > contracts;
                // std::map<THASH_id, REF_getter<MsgData::TX> >  transaction_pool_of_leader;

    // return sz;

}
bool Node::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;

    switch (e->tid)
    {
    case timers::TIMER_SYNC_TIMEDOUT:
        logNode("FAILED SYNC, NODE STOPPED---------------------------------------------------------------");
    break;
    case timers::TIMER_VALIDATE_BLOCK_DELAY:
    {
        if (state_Z != STATE_NORMAL)
            return true;
        auto &li = l_blocks[prev_root_hash_Z()].leader_info;
        // auto &li = hbs.leader_info;
        do_start_block();
        logNode("do_start_block();");
        li.transaction_responders.clear();
        return true;
        /*
                if (hbs.leader_info.leader_cert_2.valid() && hbs.leader_info.leader_cert_2->nodes.size() == li.transaction_responders.size())
        {
            do_start_block();
            logNode("do_start_block();");
            li.transaction_responders.clear();
        }
*/
    }
    case timers::TIMER_RESTART_BLOCK:
    {
        if (state_Z != STATE_NORMAL)
            return true;
        auto &li = l_blocks[prev_root_hash_Z()].leader_info;
        // auto &li = hbs.leader_info;
        li.request_for_transactions_sent = true;
        do_request_for_transactions(li);
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
            const rpcEvent::IncomingOnAcceptor *ev = static_cast<const rpcEvent::IncomingOnAcceptor *>(e.get());
            auto &IDA = ev->e->id;

            switch (IDA)
            {
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
            const rpcEvent::IncomingOnConnector *ev = static_cast<const rpcEvent::IncomingOnConnector *>(e.get());
            auto &IDC = ev->e->id;
            switch (IDC)
            {
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
        auto cc = getByPathNoCreate(root.get(), telnet_data_path, db_state.get());
        if (cc.valid())
        {
            // sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, cc->dump() + "\n", this));
        }
    }
    if (match(go, e->command, tokens))
    {

        sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "if(match(go, e->command, tokens)) " + std::to_string(tokens.size()) + "\n", this));
        if (tokens.size() == 2)
        {
            sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "if(tokens.size()==2)\n", this));
            telnet_data_path.push_back(tokens[1]);
            auto cc = getByPathNoCreate(root.get(), telnet_data_path, db_state.get());
            if (cc.valid())
            {
                sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "OK, current path: " + cc->getDbId() + "\n", this));
            }
            else
            {
                sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "FAILURE, cannot change path\n", this));
                telnet_data_path.pop_back();
            }
        }
    }
    if (match(back, e->command, tokens))
    {
        telnet_data_path.pop_back();
        auto cc = getByPathNoCreate(root.get(), telnet_data_path, db_state.get());
        if (cc.valid())
        {
            sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "OK, current path: " + cc->getDbId() + "\n", this));
        }
        else
        {
            sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, "FAILURE, cannot change path\n", this));
        }
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
      iInstance(ins),
      DBH_feature(ins)
{
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
    // auto c = getByPathNoCreate(root.get(), da, db_state.get());
    // if (!c.valid())
    // {
    //     r.make_response("<pre> if(!c.valid()) </pre>");
    //     return true;
    // }
    nlohmann::json j;
    dump(j);
    // auto buf = c->dump();
    r.make_response("<pre>" + j.dump(2) + "</pre>");
    return true;
}

void Node::Service::do_request_for_transactions( heart_beat_node_info& li)
{
    MUTEX_INSPECTOR;

    REF_getter<MsgData::GetTransactionREQ> rt = new MsgData::GetTransactionREQ();
    if(!li.leader_cert_2.valid())
    {
        throw CommonError("if(!li.leader_cert_2.valid())");
    }
    rt->lc = li.leader_cert_2;
    li.request_for_transactions_time = iUtils->getNow();
    broadcast_MsgEvent(rt.get());
}

// #include "sql"
BLOCK_id Node::Service::execute_block(b_params &b,  const REF_getter<MsgData::HeartBeatREQ> &lc)
{
    MUTEX_INSPECTOR;
    M_LOCK(root->state_mutex);
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
            auto u = root->getAddressState(senderAddress,NULL,db_state.get());
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

    auto rh=proceed_merkle_on_transaction_pool_hashers(root);
    calc_fee_rewards_nodes(b, lc);

    rh=proceed_merkle_on_transaction_pool_hashers(root);
    return rh;
}
void Node::Service::calc_fee_rewards_nodes(b_params &b, const REF_getter<MsgData::HeartBeatREQ> &lc)
{
    MUTEX_INSPECTOR;

    double total_staked=0;
    auto nn=root->getAllNodes(db_state.get());
    for(auto& n:nn)
    {
        // total_staked+=n->get_full_stake();
    }
    auto local_prev_block=prev_block;
    std::set<NODE_id> ns;
    if(local_prev_block.valid())
    {
        for(auto& z:local_prev_block->node_validators)
        {
            ns.insert(z);
            auto n=root->getNode(z,db_state.get());
            total_staked+=n->get_full_stake_DBL();
        }
        for(auto& z:local_prev_block->node_validators)
        {
            auto n=root->getNode(z,db_state.get());
            // n->get_full_stake();
            auto portion=n->get_full_stake_DBL()*b.node_rewards/total_staked;
            auto u = root->getAddressState(n->get_owner(),NULL,db_state.get());
            {
                M_LOCK(u->parent->mx);
                u->balance+=portion;
            }
            u->setDirty(NULL);
            b.emit_block("reward",R"({"node":"%s","fee":"%s"})",z.container.c_str(),portion.toString().c_str());
        }
        
    }


    b.emit_block("total_fee",R"({"fee":"%s"})",b.node_rewards.toString().c_str());
}

BLOCK_id Node::Service::proceed_merkle_on_transaction_pool_hashers(const REF_getter<root_data> &r)
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
    db_to_save_Z.add("#root#", root_buf);
    // db_to_save_Z.add("#root_hash#", root_hash.container);
    BLOCK_id ret;
    ret.container = root_hash.container;
    return ret;
}
#include <stdlib.h>
int Node::Service::nodeDistanceToLeader(const NODE_id &node)
{
    auto nv = root->getAllNodes(db_state.get());
    auto rh=prev_root_hash_Z();
    int crc = __crc32(0, rh.container.data(), rh.container.size());
    int idx = crc % nv.size();
    int npoz = -1;
    for (int i = 0; i < nv.size(); i++)
    {
        if (node == nv[i]->getName())
            npoz = i;
    }
    return abs(idx - npoz);
}
bool Node::Service::isNodeGreaterOrEqual(const NODE_id &nodeLeft, const NODE_id &nodeRight)
{
    if (nodeLeft == nodeRight)
        return true;

    auto nv = root->getAllNodes(db_state.get());
    std::sort(nv.begin(), nv.end(), [](const REF_getter<bc_node>& a, const REF_getter<bc_node>& b)
    {
        return a->getName() < b->getName();
    });

    // ФИКС 1: uint32_t вместо int, чтобы избежать отрицательного crc
    auto prev_rh=prev_root_hash_Z();
    uint32_t crc = __crc32(0, prev_rh.container.data(), prev_rh.container.size());
    int idx = static_cast<int>(crc % nv.size());

    int npoz = -1;
    int tpoz = -1;
    for (int i = 0; i < static_cast<int>(nv.size()); i++)
    {
        if (nodeLeft == nv[i]->getName())
            npoz = i;
        if (nodeRight == nv[i]->getName())
            tpoz = i;
    }

    // ФИКС 2: защита от ненайденных нод
    if (npoz == -1 || tpoz == -1)
    {
        return npoz != -1; // если nodeLeft найден, а nodeRight нет — nodeLeft лучше
    }

    int distLeft  = abs(idx - npoz);
    int distRight = abs(idx - tpoz);

    // ФИКС 3: TIE-BREAKER при равных расстояниях
    if (distLeft == distRight)
    {
        return nodeLeft < nodeRight;  // побеждает нода с меньшим именем
    }

    return distLeft < distRight;
}
bool Node::Service::verify_block(const REF_getter<MsgData::BlockAcceptedREQ> &lc)
{
    /// проверка сертификата лидера
    if(!lc.valid())
        return false;
    {
        MUTEX_INSPECTOR;
        std::vector<blst_cpp::PublicKey> agg_pk;
        double stake;
        for (auto &z : lc->node_validators)
        {
            auto n = root->getNode(z,db_state.get());
            if (!n.valid())
            {
                logErr2("            if (!n.valid()) %s",z.container.c_str());
                return false;

            }
            agg_pk.push_back(n->get_bls_pk());
            stake += n->get_full_stake_DBL();
        }
        auto nn=root->getAllNodes(db_state.get());
        double ts = 0;
        for(auto &z: nn)
        {
            ts+=z->get_full_stake_DBL();
        }
        if (stake < ts * QUORUM)
        {
            logErr2("verify lc quorum failed");
            return false;
        }
        // throw CommonError("if(stake.toDouble() < root->getValues(NULL)->total_staked.toDouble() * QUORUM)");
        if (!lc->agg_sig.verify(agg_pk, blake2b_hash(lc->blockInfo->getBuffer()).container))
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
        do_heart_beat();
    }
    return true;
}
bool Node::Service::LcEnvelopeREQ(const MsgData::LcEnvelopeREQ* m, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    // logNode("LcEnvelopeREQ 33333dffffff");
    
    inBuffer in(m->msg);
    auto id = in.get_PN();
    REF_getter<MsgData::Base> msg = msgFactory.create(id);
    msg->unpack(in);
    
    REF_getter<MsgData::BlockAcceptedREQ> lc;
    if(m->prev_lc.size())
    {
        lc=new MsgData::BlockAcceptedREQ;
        inBuffer in2(m->prev_lc);
        lc->unpack2(in2);

    }
    
    switch (msg->type)
    {
    case msgid::HeartBeatREQ:
    
        // last_activity_time=iUtils->getNow();
        return HeartBeatREQ(static_cast<const MsgData::HeartBeatREQ *>(msg.get()),lc.valid()?lc.get():NULL, src_node, route);

    default:
        throw CommonError("2 MsgData %s", msgName(msg->type));
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
    auto n = root->getNode(m->node_signer,db_state.get());
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
    case msgid::DoYouHaveBlockREQ:
        return DoYouHaveBlockREQ(static_cast<const MsgData::DoYouHaveBlockREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::LcEnvelopeREQ:
        return LcEnvelopeREQ(static_cast<const MsgData::LcEnvelopeREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::GetTransactionREQ:
        return GetTransactionREQ(static_cast<const MsgData::GetTransactionREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::ValidateBlockREQ:
        last_activity_time=iUtils->getNow();
        return ValidateBlockREQ(static_cast<const MsgData::ValidateBlockREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::BlockAcceptedREQ:
        last_activity_time=iUtils->getNow();
        return BlockAcceptedREQ(static_cast<const MsgData::BlockAcceptedREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::GetSavedBlocksREQ:
        return GetSavedBlocksREQ(static_cast<const MsgData::GetSavedBlocksREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::ConfirmLeaderREQ:
        return ConfirmLeaderREQ(static_cast<const MsgData::ConfirmLeaderREQ *>(msg.get()), m->node_signer, m->route);
    // case msgid::LcREQ:
    //     return LcREQ(static_cast<const MsgData::LcREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::DelayNotificationREQ:
        return DelayNotificationREQ(static_cast<const MsgData::DelayNotificationREQ *>(msg.get()), m->node_signer, m->route);

    default:
        throw CommonError("unjandled3 MsgData %s", msgName(msg->type));
    }

    return true;
}

bool Node::Service::NodeMsgRSP(const bcEvent::NodeMsgRSP *m)
{
    auto n = root->getNode(m->node_signer,db_state.get());
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
    case msgid::DoYouHaveBlockRSP:
        return DoYouHaveBlockRSP(static_cast<const MsgData::DoYouHaveBlockRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::HeartBeatRSP:
        return HeartBeatRSP(static_cast<const MsgData::HeartBeatRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::ConfirmLeaderRSP:
        return ConfirmLeaderRSP(static_cast<const MsgData::ConfirmLeaderRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::GetTransactionRSP:
        return GetTransactionRSP(static_cast<const MsgData::GetTransactionRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::ValidateBlockRSP:
        return ValidateBlockRSP(static_cast<const MsgData::ValidateBlockRSP *>(ee.get()), m->node_signer, m->route);
    case msgid::GetSavedBlocksRSP:
        return GetSavedBlocksRSP(static_cast<const MsgData::GetSavedBlocksRSP *>(ee.get()), m->node_signer, m->route);
    // case msgid::LcRSP:
    //     return LcRSP(static_cast<const MsgData::LcRSP *>(ee.get()), m->node_signer, m->route);
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
            // yyjson::Value item = root[ii];
            // auto contract = item/"contract";
            // auto method = item/"method";
            // auto params= item/"params";
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
                else if (meth == "contract_deploy")
                    err = TR::execute_contract_deploy(params,b, t,  index);
                else if (meth == "contract_update")
                    err = TR::execute_contract_update(params, b,t,  index);
                else
                {
                    MUTEX_INSPECTOR;
                    return "unhandled method '"+method_str+"' for root contract";
                    // b.emit_command(t.tx_id, index, "error", 
                    //     R"({"error":"unhandled method %s for root contract"})", 
                    //     method_str.c_str());
                }
                if (err)
                {
                    MUTEX_INSPECTOR;
                    return err;
                    // b.emit_command(t.tx_id, index, "error", 
                    //     R"({"error":"%s"})", 
                    //     err->c_str());                
                }

            }
            else if(!err)
            {
                MUTEX_INSPECTOR;
                CONTRACT_id c;
                c.container=contract_str;
                auto m=method;
                // execute_contract(c,m,params);
                /// exec js contract
                
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
    BigInt gasLimit=0;
    BigInt gasPrice=0;
    BigInt value=0;
    
    yyjson_val *jroot=yyjson_doc_get_root(tx->doc);

    yyjson_val * j_tx = yyjson_obj_get(jroot,"tx");
    if(!j_tx)
        throw CommonError("if(!j_tx)");
    // BigInt gasLimit=0;
    // BigInt gasPrice=0;
    // BigInt value=0;
    auto err=yy_get_bn(jroot,"value",value);
    if(!err)
        err=yy_get_bn(jroot,"gasLimit",gasLimit);
    if(!err)
        err=yy_get_bn(jroot,"gasPrice",gasPrice);
    if(err)
    {
        b.emit_tx(tx_id,"error",R"({"error":"%s"})",err->c_str());
        return err;
    }    
    Rollback roll;
    t_params t(root);
    t.senderAddress=senderAddress;
    t.tx=tx;
    t.epoch=epoch;
    t.tx_id=tx_id;
    t.roll=&roll;
    t.value=value;
    t.gasLimit=gasLimit;
    auto uu=root->getAddressState(senderAddress,NULL,db_state.get());
    if(!uu.valid())
    throw CommonError("if(!uu.valid())");
    {
        M_LOCK(uu->parent->mx);
        if(uu->balance < gasLimit*gasPrice+value)
            return "not enough funds to reserve gasLimit*gasPrice+value";
    }
    /// сбрасываем все изменения состояния перед транзакцией
    // _db_to_save db_dump0;
    root->calc_tree_hash(db_to_save_Z);

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

        auto u=root->getAddressState(t.senderAddress,NULL,db_state.get());
        M_LOCK(u->parent->mx);
        u->balance-=gu*gasPrice;
        b.node_rewards+=gu*gasPrice;
        
        return err;

    }
    if(t.value<0)
    {
        t.gasUsed+=t.roll->size();
        t.rollback();
        b.emit_tx(t.tx_id,"error",R"({"error":"value exceeds limit"})");
        auto u=root->getAddressState(t.senderAddress,NULL,db_state.get());
        M_LOCK(u->parent->mx);
        u->balance-=t.gasUsed*gasPrice;
        b.node_rewards+=t.gasUsed*gasPrice;
        return "value exceeds limit";
    }
    if(t.gasUsed>gasLimit)
    {
        t.rollback();
        b.emit_tx(t.tx_id,"error",R"({"error":"gas exceeds limit"})");
        auto u=root->getAddressState(t.senderAddress,NULL,db_state.get());
        M_LOCK(u->parent->mx);
        u->balance-=gasLimit*gasPrice;
        b.node_rewards+=gasLimit*gasPrice;
        return "gas exceeds limit";
    }

    _db_to_save db_dump;
    root->calc_tree_hash(db_dump);
    size_t sz=db_dump.size();
    t.gasUsed+=sz;
    if(t.gasUsed>gasLimit)
    {

        t.rollback();
        b.emit_tx(t.tx_id,"error",R"({"error":"gas exceeds limit"})");
        auto u=root->getAddressState(t.senderAddress,NULL,db_state.get());
        M_LOCK(u->parent->mx);
        u->balance-=gasLimit*gasPrice;
        b.node_rewards+=gasLimit*gasPrice;
        return "gas exceeds limit";
    }
    // OK
    auto u=root->getAddressState(t.senderAddress,NULL,db_state.get());
    {
        M_LOCK(u->parent->mx);
        u->balance-=t.gasUsed*gasPrice+value-t.value;
    }

    root->calc_tree_hash(db_dump);
    db_to_save_Z.add(db_dump);
    
    // logErr2("user sz %d",sz);
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
    auto c=root->getContract(contract,db_state.get());
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

void Node::Service::logNode(const char *fmt, ...)
{

    // auto prev=root->getEpoch(NULL,db_state.get())->prev_block;
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
