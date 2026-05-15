#include "Events/System/Net/httpEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "base62.h"
#include "commonError.h"
#include "Events/Tools/telnetEvent.h"
#include "bigint.h"
#include "TRANSACTION_id.h"
#include "blake2bHasher.h"
#include "REF.h"
#include "Events/Tools/webHandlerEvent.h"
#include "IUtils.h"
#include "SERVICE_id.h"
#include "IInstance.h"
#include "broadcaster.h"
#include "THASH_id.h"
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
#include "CDatabase.h"
#include "getenv2.h"
#include "httpConnection.h"
#include "fee_calcer.h"
#include "init_root.h"
#include "execute_transaction.h"
#include "root_contract.h"
#include "listenerBase.h"
#include "msg.h"
#include "ioBuffer.h"
#include "unknown.h"
#include "listenerBuffered1Thread.h"
#include "t_params.h"
#include <SQLiteCpp/Database.h>

bool Node::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;

    SECURE sec;
    sec.use_ssl = false;
    for (auto &z : web_addr)
        sendEvent(ServiceEnum::HTTP, new httpEvent::DoListen(z, sec, this));

    db = new CDatabase(rocksdb_path);
    if (!root.valid())
        root = getRoot(db.get());

    init_root(root);
    initDB();
    my_sk_bls.deserializeBase62Str(getenv2(my_sk_bls_env_key));

    my_sk_ed = base62::decode(getenv2(my_sk_ed_env_key));
    logErr2("ServiceInit nodename %s", this_node_name.container.c_str());
    sendEvent(ServiceEnum::BlockValidator, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db, this));
    sendEvent(ServiceEnum::TxValidator, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db, this));
    sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db, this));
    sendEvent(ServiceEnum::GrainReader, new bcEvent::ServiceInit(my_sk_bls, my_sk_ed, this_node_name, db, this));
    for (auto &z : rpc_addr)
    {
        SECURE sec;
        sec.use_ssl = false;
        sendEvent(ServiceEnum::RPC, new rpcEvent::DoListen(z, sec));
    }
    sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));

    std::string res;
    int err = db->get_cell("#root_hash#", &res);
    if (!err)
    {
        logNode("prev_block_hash_Z.container = res;");
        prev_block_hash_Z.container = res;
    }

    logNode("do_heart_beat in startService");
    do_heart_beat();

    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^ds$", "show current element dump", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^go\\s+(.+)$", "go to child element", ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("", "^back$", "go to parent", ListenerBase::serviceId));

    msgFactory.registerMsg(msgid::HeartBeatREQ, MsgEvt::HeartBeatREQ::construct);
    msgFactory.registerMsg(msgid::HeartBeatRSP, MsgEvt::HeartBeatRSP::construct);
    msgFactory.registerMsg(msgid::GetTransactionREQ, MsgEvt::GetTransactionREQ::construct);
    msgFactory.registerMsg(msgid::GetTransactionRSP, MsgEvt::GetTransactionRSP::construct);
    msgFactory.registerMsg(msgid::ValidateBlockREQ, MsgEvt::ValidateBlockREQ::construct);
    msgFactory.registerMsg(msgid::ValidateBlockRSP, MsgEvt::ValidateBlockRSP::construct);
    msgFactory.registerMsg(msgid::BlockAcceptedREQ, MsgEvt::BlockAcceptedREQ::construct);
    msgFactory.registerMsg(msgid::BlockAcceptedRSP, MsgEvt::BlockAcceptedRSP::construct);
    msgFactory.registerMsg(msgid::GetSavedBlocksREQ, MsgEvt::GetSavedBlocksREQ::construct);
    msgFactory.registerMsg(msgid::GetSavedBlocksRSP, MsgEvt::GetSavedBlocksRSP::construct);
    msgFactory.registerMsg(msgid::DoHeartBeatREQ, MsgEvt::DoHeartBeatREQ::construct);
    msgFactory.registerMsg(msgid::ConfirmLeaderREQ, MsgEvt::ConfirmLeaderREQ::construct);
    msgFactory.registerMsg(msgid::ConfirmLeaderRSP, MsgEvt::ConfirmLeaderRSP::construct);

    do_heart_beat();

    return true;
}

void Node::Service::collectTransactions()
{
    MUTEX_INSPECTOR;
    std::map<std::string /*user addr*/, std::map<BigInt /*nonce*/, std::vector<TRANSACTION_body>>> ordered;
    for (auto &z : transaction_pool_of_leader)
    {
        msg::user_message_req ur(z.second);
        ordered[ur.address_pk_ed][ur.nonce].push_back(z.second);
    }
    transaction_pool_of_leader.clear();
    for (auto &x : ordered)
    {
        for (auto &y : x.second)
        {
            for (auto &z : y.second)
                transaction_pool_of_leader.insert({blake2b_hash(z.container), z});
        }
    }
}

void Node::Service::initDB()
{
    SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    // dbs.exec("CREATE TABLE IF NOT EXISTS blocks ("
    //          "epoch INTEGER, "
    //          "data BLOB NOT NULL)");

    dbs.exec(R"( CREATE TABLE IF NOT EXISTS  blocks (
    epoch INTEGER UNIQUE,
    prev_root_hash BLOB NOT NULL,
    data BLOB NOT NULL,
    date INTEGER NOT NULL
        )
        )");
    dbs.exec("CREATE INDEX IF NOT EXISTS   idx_prev_hash ON blocks(prev_root_hash);");
    dbs.exec("CREATE INDEX IF NOT EXISTS idx_epoch ON blocks(epoch)");

}

void Node::Service::do_start_block()
{
    MUTEX_INSPECTOR;
    // logNode("transaction_pool_of_leader sz %d", transaction_pool_of_leader.size());
    if (transaction_pool_of_leader.empty())
    {
        sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(timers::TIMER_RESTART_BLOCK, NULL, NULL, 0.5, this));
        return;
    }
    auto &hbs = blocks_leader[prev_block_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    // if(hbs.node_leader==this_node_name)
    {
        {
            make_leader_certificate();
            REF_getter<MsgEvt::ValidateBlockREQ> b = new MsgEvt::ValidateBlockREQ();
            // msg::block_request b;
            b->leader_cert = li.leader_cert_2;

            auto &bt = blocks_leader[prev_block_hash_Z];

            collectTransactions();

            for (auto &z : transaction_pool_of_leader)
                b->transaction_bodies.push_back(z.second);
            // transaction_pool_of_leader.clear();

            outBuffer ob;
            b->pack(ob);
            msg::node_message_ed nm(ob.asString()->container, this_node_name, my_sk_ed);

            sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(), ListenerBase::serviceId));
        }
    }
}
bool Node::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool Node::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;

    switch (e->tid)
    {
    case timers::TIMER_RESTART_BLOCK:
    {
        if(state_Z!=NORMAL)
        return true;
        auto &hbs = blocks_leader[prev_block_hash_Z].heart_beat_store;
        auto &li = hbs.leader_info;
        li.request_for_transactions_sent = true;
        do_request_for_transactions(li);
        return true;
    }
    break;
    case timers::TIMER_START_HEART_BEAT:
    {
        if(state_Z!=NORMAL)
        return true;

        DBG(logNode("case timers::TIMER_START_HEART_BEAT:"));

        do_heart_beat();
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
    // if(e->route.size())
    // {
    //     passEvent(e);
    //     return true;
    // }
    try
    {
        MUTEX_INSPECTOR;
        auto &ID = e->id;
        switch (ID)
        {
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
        case bcEventEnum::Msg:
            return Msg(static_cast<const bcEvent::Msg *>(e.get()), false);
        case bcEventEnum::MsgReply:
            return MsgReply(static_cast<const bcEvent::MsgReply *>(e.get()), false);
        case httpEventEnum::RequestIncoming:
            return RequestIncoming(static_cast<const httpEvent::RequestIncoming *>(e.get()));
        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor *ev = static_cast<const rpcEvent::IncomingOnAcceptor *>(e.get());
            auto &IDA = ev->e->id;

            switch (IDA)
            {
            case bcEventEnum::Msg:
                return Msg(static_cast<const bcEvent::Msg *>(ev->e.get()), true);
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply *>(ev->e.get()), true);
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
            case bcEventEnum::Msg:
                return Msg(static_cast<const bcEvent::Msg *>(ev->e.get()), true);
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply *>(ev->e.get()), true);

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
    logErr2("telnet command %s", e->command.c_str());
    std::vector<std::string> tokens;
    auto ds = "^ds$";
    auto go = "^go\\s+(.+)$";
    auto back = "^back$";

    if (match(ds, e->command, tokens))
    {
        auto cc = getByPathNoCreate(root.get(), telnet_data_path, db.get());
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
            auto cc = getByPathNoCreate(root.get(), telnet_data_path, db.get());
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
        auto cc = getByPathNoCreate(root.get(), telnet_data_path, db.get());
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
    logErr2("RequestIncoming %s", e->req->url.c_str());
    HTTP::Response r(e->req);
    auto uri = (std::string)e->req->url;
    auto da = iUtils->splitString("/", uri);
    auto c = getByPathNoCreate(root.get(), da, db.get());
    if (!c.valid())
    {
        r.make_response("<pre> if(!c.valid()) </pre>");
        return true;
    }

    auto buf = c->dump();
    // r.request->
    r.make_response("<pre>" + buf + "</pre>");
    return true;
}

// void Node::Service::on_blockResponse(const msg::block_response& br)
void Node::Service::do_request_for_transactions(const Node::heart_beat_node_info &li)
{
    // logNode("@@ %s", __FUNCTION__);
    MUTEX_INSPECTOR;
    REF_getter<MsgEvt::GetTransactionREQ> rt = new MsgEvt::GetTransactionREQ();
    if (!li.leader_cert_2.valid())
        throw CommonError("if(!li.leader_cert.valid())");
    rt->lc = li.leader_cert_2;
    msg::node_message_ed nm(rt->getBuffer(), this_node_name, my_sk_ed);
    sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(), ListenerBase::serviceId));
}

// #include "sql"
void Node::Service::resetTimer()
{
    sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));
}
void Node::Service::execute_block(t_params &t,const REF_getter<root_data> &rt, const BLOCK_id &bl, const std::vector<TRANSACTION_body> &trs, const std::vector<NODE_id> &nodes_in_leader_cert)
{
    MUTEX_INSPECTOR;

    outBuffer o;
    t.instruction_reports.resize(trs.size());
    for (int ti = 0; ti < trs.size(); ti++)
    {
        std::optional<std::string> t_err;
        THASH_id th = blake2b_hash(trs[ti].container);
        msg::user_message_req ur(trs[ti]);
        REF_getter<fee_calcer> by = t.feeCalcers.get(ur.address_pk_ed);
        // if(!u.valid())
        //     throw CommonError("if(!u.valid()) %s %d",__FILE__,__LINE__);
        if (!ur.verify())
            t_err = "verify failed";
        // // BigInt nonce=0;
        if (!t_err)
        {
            auto u = rt->getUserState(ur.address_pk_ed);
            if (!u.valid())
            {
                t_err = "sender invalid";
            }
            if (!t_err)
            {
                if (u->nonce != ur.nonce)
                    t_err = "invalid nonce";
                if (!t_err)
                {
                    t.instruction_reports[ti].resize(ur.payload.size());
                    execute_transaction(ti, t, ur.address_pk_ed, ur.payload, by);
                    u->nonce += 1;
                    u->setDirty(by);
                }
            }
        }
        if (!t_err)
            t.setTxSuccess(th);
        else
            t.setTxError(th, *t_err);
    }

}
REF_getter<MsgEvt::BlockDBStore> Node::Service::prepareBlockDBStore(const std::vector<TRANSACTION_body> &trs, const t_params& t, const std::vector<NODE_id> &nodes_in_leader_cert)
{
    // if (!prepared_block.valid())
    REF_getter<MsgEvt::BlockDBStore>    pb = new MsgEvt::BlockDBStore;
    pb->epoch = root->getEpoch()->epoch;
    pb->att_data.transaction_reports = t.transaction_reports;
    pb->att_data.trs = trs;

    auto newEpoch = root->getEpoch();
    newEpoch->epoch += 1;
    newEpoch->setDirty(NULL);

    auto new_root_hash = proceed_merkle_on_transaction_pool_hashers(root);

    BigInt total_fees;
    for (auto &z : t.feeCalcers.calcers)
    {
        auto u = root->getUserState(z.first);
        if (!u.valid())
            throw CommonError("if(!u.valid()) 334455");
        if (u->balance < z.second->get_fee())
        {
            u->balance = 0;
            u->setDirty(NULL);
        }
        else
        {
            logErr2("balance deduct %s fee %s", u->balance.toString().c_str(), z.second->get_fee().toString().c_str());
            u->balance -= z.second->get_fee();
            u->setDirty(NULL);
        }
        total_fees += z.second->get_fee();
        pb->att_data.fees[z.first] = z.second->get_fee();
        z.second->reset();
    }
    BigInt total_rewards = (total_fees * 9) / 10;
    for (auto &n : nodes_in_leader_cert)
    {
        auto node = root->getNode(n);
        if (!node.valid())
            throw CommonError("if(!node.valid()) 556677");
        auto owner = node->owner_ed_pk;
        auto u = root->getUserState(owner);
        if (!u.valid())
        {
            throw CommonError("if(!u.valid()) 778899");
            // u=root->addUser(upk,NULL);
        }
        BigInt amt = (total_rewards * node->total_stake) / root->getValues()->total_staked;
        u->balance += amt;
        u->setDirty(NULL);
        if (n == this_node_name && amt > 0)
            logNode("node %s rewarded %s grans", n.container.c_str(), amt.toString().c_str());
        pb->att_data.rewards[n] = amt;
    }
    return pb;
}
BLOCK_id Node::Service::proceed_merkle_on_transaction_pool_hashers(const REF_getter<root_data> &r)
{
    // _db_to_save db_to_save_L;
    // auto &bt=blocks[prev_block_hash];

    r->calc_tree_hash(db_to_save_Z);
    r->calcers_Z.clear();

    auto root_buf = r->getBuffer();
    auto root_hash = blake2b_hash(root_buf);
    db_to_save_Z.add("#root#", root_buf);
    db_to_save_Z.add("#root_hash#", root_hash.container);
    BLOCK_id ret;
    ret.container = root_hash.container;
    return ret;
}
#include "__crc32.h"
#include <stdlib.h>
int Node::Service::nodeDistanceToLeader(const NODE_id &node)
{
    auto nv = root->getNodesNames();
    int crc = __crc32(0, prev_block_hash_Z.container.data(), prev_block_hash_Z.container.size());
    int idx = crc % nv.size();
    int npoz = -1;
    for (int i = 0; i < nv.size(); i++)
    {
        if (node == nv[i])
            npoz = i;
    }
    return abs(idx - npoz);
}
bool Node::Service::isNodeGreaterOrEqual(const NODE_id &nodeLeft, const NODE_id &nodeRight)
{
    if (nodeLeft == nodeRight)
        return true;
    auto nv = root->getNodesNames();
    {
        int crc = __crc32(0, prev_block_hash_Z.container.data(), prev_block_hash_Z.container.size());
        int idx = crc % nv.size();

        int npoz = -1;
        int tpoz = -1;
        for (int i = 0; i < nv.size(); i++)
        {
            if (nodeLeft == nv[i])
                npoz = i;
            if (nodeRight == nv[i])
                tpoz = i;
        }
        return abs(idx - npoz) < abs(idx - tpoz);
    }
    return 0;
}
bool Node::Service::PutTransactionREQ(const bcEvent::PutTransactionREQ *e)
{
    MUTEX_INSPECTOR;
    TRANSACTION_body tr;
    tr.container = e->msg;
    transaction_pool_of_leader.insert({blake2b_hash(e->msg), tr});
    return true;
}

void Node::Service::logNode(const char *fmt, ...)
{

    {
        va_list ap;
        va_start(ap, fmt);
        auto epoch = root->getEpoch();
        if (!epoch.valid())
        {
            throw CommonError("if(!epoch.valid())");
        }
        fprintf(stdout, "%lf [Node] [%s] [%s] [%s] ", double(iUtils->getNow()) / 1000000., this_node_name.container.c_str(), prev_block_hash_Z.str().c_str(), epoch->epoch.toString().c_str());
        vfprintf(stdout, fmt, ap);
        fprintf(stdout, "\n");
        va_end(ap);
    }
}
