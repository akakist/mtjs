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
#include "fee_calcer.h"
#include "execute_transaction.h"
#include "root_contract.h"
#include "listenerBase.h"
#include "msg.h"
#include "ioBuffer.h"
#include "s_ed.h"
#include "unknown.h"
#include "listenerBuffered1Thread.h"
#include "t_params.h"
#include <SQLiteCpp/Database.h>
#include "__crc32.h"
#include "init_root.h"
#include "nodeService.h"
#include "CDatabase.h"

bool Node::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;
    last_activity_time=iUtils->getNow();

    SECURE sec;
    sec.use_ssl = false;
    for (auto &z : web_addr)
        sendEvent(ServiceEnum::HTTP, new httpEvent::DoListen(z, sec, this));

    db_state = new CDatabase(rocksdb_path+"_state");
    auto db_state2=db_state;
    iUtils->add_shutdown_cb([db_state2]() {

        // logNode("close db handler");
        db_state2->close();
    });

    if (!root.valid())
        root = getRoot(db_state.get());

    db_history = new DB_history(rocksdb_path+"_history");
    auto db_h_copy=db_history;
    iUtils->add_shutdown_cb([db_h_copy]() {

        // logNode("close db handler");
        db_h_copy->close();
    });
    init_root(root);
    // initDB();
    my_sk_bls.deserializebase16Str(getenv2(my_sk_bls_env_key));

    my_sk_ed = base16::decode(getenv2(my_sk_ed_env_key));
    logNode("ServiceInit nodename %s", this_node_name.container.c_str());
    sendEvent(ServiceEnum::BlockValidator, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_state, root, this));
    sendEvent(ServiceEnum::TxValidator, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_state, root, this));
    sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_state, root, this));
    sendEvent(ServiceEnum::GrainReader, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db_state, root, this));
    for (auto &z : rpc_addr)
    {
        SECURE sec;
        sec.use_ssl = false;
        sendEvent(ServiceEnum::RPC, new rpcEvent::DoListen(z, sec));
    }
    // sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));
    sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(timers::TIMER_PERIODIC_CLOCK, NULL, NULL, 1., this));

    std::string res;
    int err = db_state->get_cell("#root_hash#", &res);
    if (!err)
    {
        // logNode("prev_root_hash_Z.container = res;");
        prev_root_hash_Z.container = res;
    }

    logNode("do_heart_beat in startService");
    do_heart_beat();

    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^ds$", "show current element dump", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^go\\s+(.+)$", "go to child element", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^back$", "go to parent", ListenerBase::serviceId));


    do_heart_beat();

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
        auto &nonce=z.second->nonce;
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
    auto &hbs = blocks_leader[prev_root_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    {
        {
            make_leader_certificate();
            REF_getter<MsgData::ValidateBlockREQ> b = new MsgData::ValidateBlockREQ();
            // msg::block_request b;
            b->leader_cert = li.leader_cert_2;

            auto &bt = blocks_leader[prev_root_hash_Z];
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
    auto msg=b->getBuffer();

    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    sendEvent(ServiceEnum::BroadcasterTree,
              new bcEvent::BroadcastMessage(ServiceEnum::Node,
                                            this_node_name, signature,msg, ListenerBase::serviceId));

}
bool Node::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    if(e->tid==timers::TIMER_PERIODIC_CLOCK)
    {
        auto diff=double(iUtils->getNow()-last_activity_time)/1000000.;

        // logNode("idle time sec %lf",double(iUtils->getNow()-last_activity_time)/1000000.);
        if(diff>2. && transaction_pool_of_leader.size())
        {
            do_heart_beat();
        }
        // if(iUtils->getNow()-last_activity_time)
    }
    return true;
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
        auto &hbs = blocks_leader[prev_root_hash_Z].heart_beat_store;
        auto &li = hbs.leader_info;
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
        auto &hbs = blocks_leader[prev_root_hash_Z].heart_beat_store;
        auto &li = hbs.leader_info;
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
            sendEvent(ServiceEnum::Telnet, new telnetEvent::Reply(e->socketId, cc->dump() + "\n", this));
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
}

Node::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins),
      iInstance(ins)
{
    rocksdb_path = ins->getConfig()->get_string("rockdb_path", "/db/r1", "Path to access to rocksdb");
    sqlite_pn = ins->getConfig()->get_string("sqlite_pn", "/db/1", "Pathname to access to sqlite");
    rpc_addr = ins->getConfig()->get_tcpaddr("rpc_addr", "127.0.0.1:2345", "rpc address(es) of node ex: ip:port,ip2:port2");
    web_addr = ins->getConfig()->get_tcpaddr("web_addr", "127.0.0.1:2347", "web address(es) of node ex: ip:port,ip2:port2");
    my_sk_bls_env_key = ins->getConfig()->get_string("my_sk_bls_env_key", "sk_bls_env_key", "env key of bls key");
    my_sk_ed_env_key = ins->getConfig()->get_string("my_sk_ed_env_key", "sk_ed_env_key", "env key of ed key");
    this_node_name.container = ins->getConfig()->get_string("this_node_name", "n0", "registered name of node");
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
    auto c = getByPathNoCreate(root.get(), da, db_state.get());
    if (!c.valid())
    {
        r.make_response("<pre> if(!c.valid()) </pre>");
        return true;
    }

    auto buf = c->dump();
    r.make_response("<pre>" + buf + "</pre>");
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
void Node::Service::resetTimer()
{
    // sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));
}
BLOCK_id Node::Service::execute_block(t_params &t,  const REF_getter<MsgData::LeaderCertificate> &lc)
{
    MUTEX_INSPECTOR;
    M_LOCK(root->state_mutex);
    // outBuffer o;
    for (int ti = 0; ti < t.validateBlockREQ->transaction_bodies.size(); ti++)
    {
        MUTEX_INSPECTOR;
        std::optional<std::string> t_err;
        auto tt=t.validateBlockREQ->transaction_bodies[ti];
        auto tx_hash=tt->getHash();
        auto &pk_bin=tt->pk_ed_bin;
        ADDRESS_id senderAddress;
        senderAddress.addr=blake2b_hash(pk_bin).container;
        auto &tj = tt->tx_body;
        REF_getter<fee_calcer> by = t.feeCalcers.get(senderAddress);
        if (!tt->verify())
        {
            t_err = "verify failed @12";
            logNode("verify failed @12");
        }
        if (!t_err)
        {
            MUTEX_INSPECTOR;
            auto u = root->getUserState(senderAddress);
            if (!u.valid())
            {
                t_err = "sender invalid";
                logNode("sender invalid");
            }
            if (!t_err)
            {
                if (u->getNonce() != tt->nonce)
                {
                    logNode("invalid nonce, expected %lld got %lld", u->getNonce(), tt->nonce);
                    t_err = "invalid nonce";

                }
                if (!t_err)
                {
                    MUTEX_INSPECTOR;
                    execute_transaction(tt->getHash(), t, senderAddress, tj, by, lc->heart_beat->new_epoch);
                    u->incNonce();
                    u->setDirty(lc->heart_beat->new_epoch);

                }
            }
        }
        if (!t_err)
            t.emit_tx(tx_hash, "result", R"({"success":true})");
        else
            t.emit_tx(tx_hash, "error", R"({"error":"%s"})", t_err->c_str());    
    }

    auto rh=proceed_merkle_on_transaction_pool_hashers(root);
    calc_fee_rewards_nodes(t, lc);
    auto newEpoch = root->getEpoch();
    newEpoch->epoch.container += 1;
    newEpoch->prev_lc = t.validateBlockREQ->leader_cert->getBuffer();
    newEpoch->setDirty(lc->heart_beat->new_epoch);

    rh=proceed_merkle_on_transaction_pool_hashers(root);
    return rh;
}
void Node::Service::calc_fee_rewards_nodes(t_params &t, const REF_getter<MsgData::LeaderCertificate> &lc)
{
    MUTEX_INSPECTOR;
    std::map<Cellable*, std::set<REF_getter<fee_calcer>>> cc;
    for(auto & z:t.calcers)
    {
        auto cell=z.first->parent;
        while(cell)
        {
            for(auto &x: z.second)
                cc[cell].insert(x);

            cell=cell->parent;
        }
    }
    for(auto& z: cc)
    {
        size_t size=z.first->last_size;
        size_t portion=size/z.second.size();
        for(auto &x: z.second)
        {
            x->add(portion);
        }
    }

    // auto new_root_hash = proceed_merkle_on_transaction_pool_hashers(root);
    BigInt total_staked=0;
    auto nn=root->getAllNodes();
    for(auto& n:nn)
    {
        total_staked+=n->get_full_stake();
    }


    BigInt total_fees;
    for (auto &z : t.feeCalcers.calcers)
    {
        auto u = root->getUserState(z.first);
        if (!u.valid())
            throw CommonError("if(!u.valid()) 334455");
        if (u->getBalance() < z.second->get_fee())
        {
            u->setBalance(0);
            u->setDirty(lc->heart_beat->new_epoch);
        }
        else
        {
            // logNode("balance deduct %s fee %s", u->getBalance().toString().c_str(), z.second->get_fee().toString().c_str());
            u->subBalance(z.second->get_fee());
            u->setDirty(lc->heart_beat->new_epoch);
        }
        total_fees += z.second->get_fee();
        t.att_data->fees[z.first] = z.second->get_fee();
        t.emit_block("fee",R"({"address":"%s","fee":"%s"})",base16::encode(z.first.addr).c_str(),z.second->get_fee().toString().c_str());
        z.second->reset();
    }
    t.emit_block("total_fee",R"({"fee":"%s"})",total_fees.toString().c_str());
    // iUtils->getNow
    BigInt total_rewards = (total_fees * 9) / 10;
    for (auto &n : lc->nodes)
    {
        auto node = root->getNode(n);
        if (!node.valid())
            throw CommonError("if(!node.valid()) 556677");
        auto owner = node->get_owner();
        auto u = root->getUserState(owner);
        if (!u.valid())
        {
            throw CommonError("if(!u.valid()) 778899");
            // u=root->addUser(upk,NULL);
        }
        BigInt amt = (total_rewards * node->get_full_stake()) / total_staked;
        u->addBalance(amt);
        u->setDirty(lc->heart_beat->new_epoch);
        // if (n == this_node_name && amt > 0)
        //     logNode("node %s rewarded %s grans", n.container.c_str(), amt.toString().c_str());
        t.att_data->rewards[n] = amt;
        t.emit_block("reward",R"({"node":"%s","amount":"%s"})",n.container.c_str(), amt.toString().c_str());
    }
    std::set<NODE_id> ns;
    for(auto& z:lc->nodes)
    {
        ns.insert(z);
    }
    auto nodes=t.root->getAllNodes();
    for(auto &n: nodes)
    {
        if(ns.count(n->getName()))
        {
            if(n->get_missed_rounds()==0)
                continue;
            else
            {
                n->reset_missed_rounds();
                n->setDirty(lc->heart_beat->new_epoch);
            }
        }
        else
        {
            if(n->get_missed_rounds()>=100)
            {
                continue;
            }
            else    
            {
                n->inc_missed_rounds();
                n->setDirty(lc->heart_beat->new_epoch);
            }
        }
    }
}

BLOCK_id Node::Service::proceed_merkle_on_transaction_pool_hashers(const REF_getter<root_data> &r)
{
    MUTEX_INSPECTOR;
    r->calc_tree_hash(db_to_save_Z);
    // r->calcers_Z.clear();

    auto root_buf = r->getBuffer();
    auto root_hash = blake2b_hash(root_buf);
    db_to_save_Z.add("#root#", root_buf);
    db_to_save_Z.add("#root_hash#", root_hash.container);
    BLOCK_id ret;
    ret.container = root_hash.container;
    return ret;
}
#include <stdlib.h>
int Node::Service::nodeDistanceToLeader(const NODE_id &node)
{
    auto nv = root->getAllNodes();
    int crc = __crc32(0, prev_root_hash_Z.container.data(), prev_root_hash_Z.container.size());
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
    auto nv = root->getAllNodes();
    {
        int crc = __crc32(0, prev_root_hash_Z.container.data(), prev_root_hash_Z.container.size());
        int idx = crc % nv.size();

        int npoz = -1;
        int tpoz = -1;
        for (int i = 0; i < nv.size(); i++)
        {
            if (nodeLeft == nv[i]->getName())
                npoz = i;
            if (nodeRight == nv[i]->getName())
                tpoz = i;
        }
        return abs(idx - npoz) < abs(idx - tpoz);
    }
    return 0;
}
bool Node::Service::verify_leader_certificate(const REF_getter<MsgData::LeaderCertificate> &lc)
{
    /// проверка сертификата лидера
    {
        MUTEX_INSPECTOR;
        std::vector<blst_cpp::PublicKey> agg_pk;
        BigInt stake;
        for (auto &z : lc->nodes)
        {
            auto n = root->getNode(z);
            if (!n.valid())
            {
                logErr2("            if (!n.valid()) %s",z.container.c_str());
                return false;

            }
            agg_pk.push_back(n->get_bls_pk());
            stake += n->get_full_stake();
        }
        auto nn=root->getAllNodes();
        BigInt ts = 0;
        for(auto &z: nn)
        {
            ts+=z->get_full_stake();
        }
        if (stake.toDouble() < ts.toDouble() * QUORUM)
        {
            logErr2("verify lc quorum failed");
            return false;
        }
        // throw CommonError("if(stake.toDouble() < root->getValues(NULL)->total_staked.toDouble() * QUORUM)");
        if (!lc->agg_sig.verify(agg_pk, blake2b_hash(lc->heart_beat->getBuffer()).container))
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
    if(!stage_is_working)
    {
        stage_is_working=true;
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
    REF_getter<MsgData::LeaderCertificate> lc;
    if(m->prev_lc.size())
    {
        lc=new MsgData::LeaderCertificate;
        inBuffer in2(m->prev_lc);
        lc->unpack2(in2);

    }
    switch (msg->type)
    {
    case msgid::HeartBeatREQ:
        last_activity_time=iUtils->getNow();
        return HeartBeatREQ(static_cast<const MsgData::HeartBeatREQ *>(msg.get()),lc.valid()?lc.get():NULL, src_node, route);

    default:
        throw CommonError("2 MsgData %s", msgName(msg->type));
    }

    return true;
}


bool Node::Service::NodeMsgREQ(const bcEvent::NodeMsgREQ *m)
{
    auto n = root->getNode(m->node_signer);
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
        last_activity_time=iUtils->getNow();
        return DoYouHaveBlockREQ(static_cast<const MsgData::DoYouHaveBlockREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::LcEnvelopeREQ:
        last_activity_time=iUtils->getNow();
        return LcEnvelopeREQ(static_cast<const MsgData::LcEnvelopeREQ *>(msg.get()), m->node_signer, m->route);
    case msgid::GetTransactionREQ:
        last_activity_time=iUtils->getNow();
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
        last_activity_time=iUtils->getNow();
        return ConfirmLeaderREQ(static_cast<const MsgData::ConfirmLeaderREQ *>(msg.get()), m->node_signer, m->route);

    default:
        throw CommonError("unjandled3 MsgData %s", msgName(msg->type));
    }

    return true;
}
bool Node::Service::NodeMsgRSP(const bcEvent::NodeMsgRSP *m)
{
    auto n = root->getNode(m->node_signer);
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
    default:
        throw CommonError("unhandled22 p020 %s", msgName(id));
        break;
    }

    return true;
}

void Node::Service::logNode(const char *fmt, ...)
{

    auto epoch = root->getEpoch();
    {
        va_list ap;
        va_start(ap, fmt);
        if (!epoch.valid())
        {
            throw CommonError("if(!epoch.valid())");
        }
        fprintf(stdout, "%lf [Node] [%s] [%s] [%ld] ", double(iUtils->getNow()) / 1000000., this_node_name.container.c_str(), prev_root_hash_Z.str().c_str(), epoch->epoch.container);
        vfprintf(stdout, fmt, ap);
        fprintf(stdout, "\n");
        va_end(ap);
    }
    {
        va_list ap;
        va_start(ap, fmt);
        std::string pn=this_node_name.container+".log";
        FILE *f = fopen(pn.c_str(), "a");
        if (f)
        {
            fprintf(f, "%lf [Node] [%s] [%s] [%ld] ", double(iUtils->getNow()) / 1000000., this_node_name.container.c_str(), prev_root_hash_Z.str().c_str(), epoch->epoch.container);
            vfprintf(f, fmt, ap);
            fprintf(f, "\n");
            fclose(f);
        }
        va_end(ap);
        // fclose(f);
    }
}
