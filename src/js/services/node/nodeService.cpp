#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <time.h>
#include <map>
#include "nodeService.h"
#include "tools_mt.h"
#include "tree.h"
#include "events_nodeService.hpp"
#include "version_mega.h"
#include "tr_exec.h"
#include "CDatabase.h"
#include "s_ed.h"
#include "QUORUM.h"
#include <SQLiteCpp/Database.h>
#include "CDatabase.h"
#include "init_root.h"
#include "execute_transaction.h"

bool Node::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;

    SECURE sec;
    sec.use_ssl=false;
    for(auto &z:web_addr)
        sendEvent(ServiceEnum::HTTP, new httpEvent::DoListen(z,sec,this));

    db=new CDatabase(rocksdb_path);
    if(!root.valid())
        root=getRoot(db.get());

    init_root(root);


    char *_bls_sk_name=getenv(my_sk_bls_env_key.c_str());
    if(!_bls_sk_name)
        throw CommonError("if(!_bls_sk_name) "+my_sk_bls_env_key);
    char *_ed_sk_name=getenv(my_sk_ed_env_key.c_str());
    if(!_ed_sk_name)
        throw CommonError("if(!_bls_sk_name) "+my_sk_ed_env_key);


    my_sk_bls.init();
    my_sk_bls.deserializeHexStr(_bls_sk_name);


    my_sk_ed=iUtils->hex2bin((std::string)_ed_sk_name);


    for(auto& z: rpc_addr)
    {
        SECURE sec;
        sec.use_ssl=false;
        sendEvent(ServiceEnum::RPC,new rpcEvent::DoListen(z,sec));

    }
    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

    std::string res;
    int err=db->get_cell("#root_hash#",&res);
    if(!err)
    {
        prev_block_hash.container=res;
    }

    logNode("do_heart_beat in startService");
    do_heart_beat();

    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("","^ds$","show current element dump",ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("","^go\\s+(.+)$","go to child element",ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("","^back$","go to parent",ListenerBase::serviceId));

    return true;
}

void Node::Service::collectTransactions()
{
    std::map<std::string, std::set<THASH_id>> cnt;
    std::set<THASH_id> rm;
    for(auto& z: transaction_pool_unverified)
    {
        msg::user_message_req ur(z.second);
        cnt[ur.nick].insert(z.first);
        auto u=root->getUser(ur.nick,NULL);
        if(!u.valid())
            throw CommonError("if(!u.valid()) %s %d",__FILE__,__LINE__);
        if(u->nonce!=ur.nonce)
        {
            logErr2("tr: %s nonce %s %s",u->nonce.toString().c_str(), ur.nonce.toString().c_str());
            logErr2("tr: %s declined due invalid nonce",z.first.str().c_str());
            rm.insert(z.first);
        }

    }
    for(auto& z: rm)
    {
        transaction_pool_unverified.erase(z);
    }
    rm.clear();

    for(auto& z: cnt)
    {
        if(z.second.size()>1)
        {
            for(auto &k: z.second)
            {
                logErr2("tr: %s declined due multiple transaction from one sender",iUtils->bin2hex(z.first).c_str());
                rm.insert(k);
            }
        }
    }
    for(auto& z: rm)
    {
        transaction_pool_unverified.erase(z);
    }
    rm.clear();
}
void Node::Service::do_start_block()
{
    if(transaction_pool_unverified.empty())
    {
        DBG(logErr2("if(transaction_pool_main.empty())"));
        sendEvent(ServiceEnum::Timer,new timerEvent::SetAlarm(timers::TIMER_RESTART_BLOCK,NULL,NULL, 1,this));
        return;
    }
    auto &hbs=heart_beat_store;
    auto &li=hbs.leader_info[hbs.node_leader];
    if(hbs.node_leader==this_node_name)
    {
        {
            make_leader_certificate();
            msg::block_request b;
            b.leader_cert=li.leader_cert;

            auto & bt=blocks[prev_block_hash];

            collectTransactions();

            for(auto& z: transaction_pool_unverified)
                b.transaction_bodies.push_back(z.second);
            transaction_pool_unverified.clear();


            msg::node_message_ed nm(b.getBuffer(),this_node_name,my_sk_ed);


            make_broadcast_message(nm.getBuffer());
        }
    }

}
bool Node::Service::on_timer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool Node::Service::on_alarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    switch(e->tid)
    {
    case timers::TIMER_RESTART_BLOCK:
    {
        // logNode("timers::TIMER_RESTART_BLOCK");
        auto &hbs=heart_beat_store;
        auto &li=hbs.leader_info[hbs.node_leader];
        li.request_for_transactions_sent=true;
        do_request_for_transactions(li);
        return true;

    }
    break;
    case timers::TIMER_START_HEART_BEAT:
    {

        DBG(logNode("case timers::TIMER_START_HEART_BEAT:"));
        auto &hbs=heart_beat_store;
        auto &li=hbs.leader_info[hbs.node_leader];
        li.request_for_transactions_sent=false;

        do_heart_beat();
        return true;

    }
    break;
    case timers::TIMER_BROADCAST_ACK_TIMEDOUT:
    {
        MUTEX_INSPECTOR;
        TIMER_BROADCAST_ACK_TIMEDOUT_cookie *c=dynamic_cast<TIMER_BROADCAST_ACK_TIMEDOUT_cookie*>(e->cookie.get());
        if(!c) throw CommonError("if(!c) 1222447");

        logNode("TIMER_BROADCAST_ACK_TIMEDOUT %s",c->dstName_.container.c_str());

        make_broadcast_message_to_tree(c->msg,c->tree, c->route);

        return true;

    }
    break;

    }
    return false;
}


bool Node::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        switch(ID)
        {
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer*)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm*)e.get());
        case webHandlerEventEnum::RequestIncoming:
            return on_RequestIncoming((const webHandlerEvent::RequestIncoming*)e.get());
        case telnetEventEnum::CommandEntered:
            return on_CommandEntered((const telnetEvent::CommandEntered*)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService*)e.get());
        case bcEventEnum::ClientMsg:
            return ClientMsg(static_cast<const bcEvent::ClientMsg*>(e.get()));
        case bcEventEnum::ClientTxSubscribeREQ:
            return ClientTxSubscribeREQ(static_cast<const bcEvent::ClientTxSubscribeREQ*>(e.get()));
        case bcEventEnum::Msg:
            return Msg(static_cast<const bcEvent::Msg*>(e.get()));
        case bcEventEnum::MsgReply:
            return MsgReply(static_cast<const bcEvent::MsgReply*>(e.get()));
        case httpEventEnum::RequestIncoming:
            return RequestIncoming(static_cast<const httpEvent::RequestIncoming*>(e.get()));
        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor*ev=static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get());
            auto &IDA=ev->e->id;

            switch(IDA)
            {
            case bcEventEnum::ClientMsg:
                return ClientMsg(static_cast<const bcEvent::ClientMsg*>(ev->e.get()));
            case bcEventEnum::ClientTxSubscribeREQ:
                return ClientTxSubscribeREQ(static_cast<const bcEvent::ClientTxSubscribeREQ*>(ev->e.get()));
            case bcEventEnum::Msg:
                return Msg(static_cast<const bcEvent::Msg*>(ev->e.get()));
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply*>(ev->e.get()));
            default:
                throw CommonError("unhabdled ev %d %s",IDA, iUtils->genum_name(IDA));
            }
        }
        break;
        case rpcEventEnum::IncomingOnConnector:
        {
            const rpcEvent::IncomingOnConnector*ev=static_cast<const rpcEvent::IncomingOnConnector*>(e.get());
            auto &IDC=ev->e->id;
            switch(IDC)
            {
            case bcEventEnum::ClientMsg:
                return ClientMsg(static_cast<const bcEvent::ClientMsg*>(ev->e.get()));
            case bcEventEnum::ClientTxSubscribeREQ:
                return ClientTxSubscribeREQ(static_cast<const bcEvent::ClientTxSubscribeREQ*>(ev->e.get()));
            case bcEventEnum::Msg:
                return Msg(static_cast<const bcEvent::Msg*>(ev->e.get()));
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply*>(ev->e.get()));

            default:
                throw CommonError("unhabdled ev %d %s",IDC, iUtils->genum_name(IDC));
            }
        }
        break;

        default:
            throw CommonError("unhabdled ev %d %s",ID, iUtils->genum_name(ID));
        }



    } catch(std::exception &e)
    {
        logNode("Node std::exception  %s",e.what());
    }
    XPASS;
    return false;
}
#include <regex>
static bool match(const std::string & re, const std::string& buf, std::vector<std::string> &tokens)
{
    MUTEX_INSPECTOR;
    std::regex rgx(re);
    std::smatch match;
    if(std::regex_search(buf,match,rgx))
    {
        tokens.clear();
        for(size_t i=0; i<match.size(); i++)
        {
            tokens.push_back(match[i].str());
        }
        return true;
    }
    return false;
}
bool Node::Service::on_CommandEntered(const telnetEvent::CommandEntered* e)
{
    logErr2("telnet command %s",e->command.c_str());
    std::vector<std::string> tokens;
    auto ds = "^ds$";
    auto go = "^go\\s+(.+)$";
    auto back="^back$";

    if(match(ds, e->command, tokens))
    {
        auto cc=getByPathNoCreate(root.get(),telnet_data_path,db.get(),NULL);
        if(cc.valid())
        {
            sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,cc->dump()+"\n",this));
        }
    }
    if(match(go, e->command, tokens))
    {

        sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"if(match(go, e->command, tokens)) "+std::to_string(tokens.size())+"\n",this));
        if(tokens.size()==2)
        {
            sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"if(tokens.size()==2)\n",this));
            telnet_data_path.push_back(tokens[1]);
            auto cc=getByPathNoCreate(root.get(),telnet_data_path,db.get(),NULL);
            if(cc.valid())
            {
                sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"OK, current path: "+cc->getDbId()+"\n",this));
            }
            else
            {
                sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"FAILURE, cannot change path\n",this));
                telnet_data_path.pop_back();

            }
        }
    }
    if(match(back, e->command, tokens))
    {
        telnet_data_path.pop_back();
        auto cc=getByPathNoCreate(root.get(),telnet_data_path,db.get(),NULL);
        if(cc.valid())
        {
            sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"OK, current path: "+cc->getDbId()+"\n",this));
        }
        else
        {
            sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"FAILURE, cannot change path\n",this));

        }
    }

    sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"NodeService received command: "+e->command+"\n",this));

    return true;
}

Node::Service::~Service()
{
}


Node::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins),
    iInstance(ins)
{
    rocksdb_path=ins->getConfig()->get_string("rockdb_path","/db/r1","Path to access to rocksdb");
    sqlite_pn=ins->getConfig()->get_string("sqlite_pn","/db/1","Pathname to access to sqlite");
    rpc_addr=ins->getConfig()->get_tcpaddr("rpc_addr","127.0.0.1:2345","rpc address(es) of node ex: ip:port,ip2:port2");
    web_addr=ins->getConfig()->get_tcpaddr("web_addr","127.0.0.1:2347","web address(es) of node ex: ip:port,ip2:port2");
    my_sk_bls_env_key=ins->getConfig()->get_string("my_sk_bls_env_key","sk_bls_env_key","env key of bls key");
    my_sk_ed_env_key=ins->getConfig()->get_string("my_sk_ed_env_key","sk_ed_env_key","env key of ed key");
    this_node_name.container=ins->getConfig()->get_string("this_node_name","n0","registered name of node");
}

bool Node::Service::on_RequestIncoming(const webHandlerEvent::RequestIncoming* )
{
    return true;
}
void registerNodeService(const char* pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::Node,"Node",getEvents_nodeService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::Node,Node::Service::construct,"Node");
        regEvents_nodeService();
    }
    XPASS;
}





bool Node::Service::RequestIncoming(const httpEvent::RequestIncoming* e)
{
    logErr2("RequestIncoming %s",e->req->url.c_str());
    HTTP::Response r(e->req);
    auto uri=(std::string)e->req->url;
    auto da=iUtils->splitString("/",uri);
    auto c=getByPathNoCreate(root.get(),da,db.get(),NULL);
    if(!c.valid())
    {
        r.make_response("<pre> if(!c.valid()) </pre>");
        return true;

    }

    auto buf=c->dump();
    // r.request->
    r.make_response("<pre>"+buf+"</pre>");
    return true;

}
void Node::Service::addToTransactionToPool(const std::string& body)
{
    MUTEX_INSPECTOR;
    THASH_id h=blake2b_hash(body);
    auto & bt=blocks[prev_block_hash];
    if(transaction_pool_unverified.count(h))
    {
        logNode("transaction already added");
        return;
    }
    TRANSACTION_body t;
    t.container=body;
    transaction_pool_unverified.insert({h,t});

}

void Node::Service::on_blockResponse(const msg::block_response& br)
{
    if(!br.verify(root->getNode(br.node_validator,NULL)->bls_pk))
    {
        logErr2("block response not validated");
        return;
    }

    msg::blockZ bl(br.payload_block);
    if(bl.prev_root_hash!=prev_block_hash)
    {
        logErr2("if(bl.prev_root_hash!=prev_block_hash)");
        return;
    }
    // if(bl.)
    auto & bt=blocks[prev_block_hash];
    if(bt.block_accepted_sent)
        return;
    // auto & bh=bt[bl.root_hash];
    bt.responses.push_back(br);
    bt.stake+=root->getNode(br.node_validator,NULL)->total_stake;
    // logNode("Block staked %lf",bt.stake.toDouble());
    if(bt.stake.toDouble() > root->getValues(NULL)->total_staked.toDouble()*QUORUM)
    {
        logNode("Block stake finalized");
        msg::block_accepted_req ba;
        if(bt.block_payload.empty())
        {
            bt.block_payload=br.payload_block;
        }
        else if(bt.block_payload!=br.payload_block)
            throw("else if(bh.block_payload!=br.payload_block)");

        ba.block_payload=bt.block_payload;
        if(bt.block_payload.empty())
            throw CommonError("if(bt.block_payload.empty())");
        ba.agg_sig.clear();
        bls::PublicKey agg_pk;
        agg_pk.clear();

        ba.leader_certificateZ=heart_beat_store.leader_info[heart_beat_store.node_leader].leader_cert;
        for(auto& z: bt.responses)
        {
            auto n=root->getNode(z.node_validator,NULL);
            agg_pk.add(n->bls_pk);
            ba.agg_sig.add(z.sig);
            ba.node_validators.push_back(z.node_validator);
        }
        if(ba.agg_sig.verify(agg_pk,blake2b_hash(ba.block_payload).container))
        {
            // logErr2("compose block_accepted test verified OK !!!!!!!!!!!!!!!!!!!!!");
        }
        else
            logErr2("block_accepted verified FAIL !!!!!!!!!!!!!!!!!!!!!");

        msg::node_message_ed nm(ba.getBuffer(),this_node_name,my_sk_ed);
        make_broadcast_message(nm.getBuffer());
        bt.block_accepted_sent=true;
    }


}
void Node::Service::do_request_for_transactions(const Node::heart_beat_node_info& li)
{
    msg::request_for_transactions rt;
    rt.payload_lc=li.leader_cert;
    msg::node_message_ed nm(rt.getBuffer(),this_node_name,my_sk_ed);
    make_broadcast_message(nm.getBuffer());

}

bool Node::Service::MsgReply(const bcEvent::MsgReply* e)
{
    inBuffer in(e->msg);


    auto p=in.get_PN();
    switch(p)
    {
    case msgid::node_message_ed:
    {
        MUTEX_INSPECTOR;
        msg::node_message_ed node_message_ed;
        node_message_ed.unpack(in);
        auto n=root->getNode(node_message_ed.src_node,NULL);
        if(!n.valid())
            throw CommonError("invalid node "+node_message_ed.src_node.container);
        if(!node_message_ed.verify(n->ed_pk))
        {
            throw CommonError("if(!node_message_ed.verify_ed_pk(n->ed_pk))");
        }
        inBuffer in2(node_message_ed.payload);
        auto p2=in2.get_PN();
        switch (p2)
        {
        case msgid::broadcast_tree_ack:
        {
            MUTEX_INSPECTOR;
            msg::broadcast_tree_ack m_broadcast_tree_ack;
            m_broadcast_tree_ack.unpack(in2);
            sendEvent(ServiceEnum::Timer,new timerEvent::StopAlarm(TIMER_BROADCAST_ACK_TIMEDOUT,toRef(m_broadcast_tree_ack.hash_buf.container),this));
            return true;
        }
        break;
        case msgid::heart_beat_rsp:
        {
            MUTEX_INSPECTOR;
            if(e->route.size())
            {
                passEvent(new bcEvent::MsgReply(e->msg, e->route));
                return true;
            }

            msg::heart_beat_rsp m_heart_beat_rsp;
            m_heart_beat_rsp.unpack(in2);
            on_heart_beat_rsp(m_heart_beat_rsp);

            return true;
        }
        break;
        case msgid::block_response:
        {
            msg::block_response br(in2);

            on_blockResponse(br);


        }
        break;
        case msgid::response_with_transactions:
        {
            msg::response_with_transactions rwt(in2);
            for(auto& z: rwt.trs)
            {
                THASH_id h=blake2b_hash(z.container);
                transaction_pool_unverified.insert({h,z});
            }
            auto &hbs=heart_beat_store;
            auto &li=hbs.leader_info[hbs.node_leader];
            li.transaction_responders.insert(node_message_ed.src_node);
            BigInt stake=0;
            for(auto &z :li.transaction_responders)
            {
                auto n=root->getNode(z,NULL);
                stake+=n->total_stake;
            }
            if(stake.toDouble() > root->getValues(NULL)->total_staked.toDouble() * QUORUM)
            {
                do_start_block();
                li.transaction_responders.clear();
            }
            return true;
        }
        break;
        case msgid::block_accepted_rsp:
        {
            msg::block_accepted_rsp bar(in2);
            if(!bar.verify(root->getNode(bar.node_signer,NULL)->bls_pk))
            {
                logErr2("block_accepted_rsp: verify failed");
                return true;
            }
            auto &bp=blocks[prev_block_hash];
            bp.acceptors.insert({bar.node_signer,bar});

            BlsAggregateSignature agg_sig;
            std::vector<BlsPublicKey> agg_pk;
            BigInt stake;
            stake=0;
            for(auto &z:bp.acceptors)
            {
                agg_sig.add(z.second.sig_bls);
                auto n=root->getNode(z.first,NULL);
                if(!n.valid())
                    throw CommonError("if(!n.valid())");

                agg_pk.push_back(n->bls_pk);
                stake+=n->total_stake;
            }
            if(!agg_sig.verify(agg_pk,bar.new_root_hash.container))
            {
                logNode("block_accepted_rsp: aggsig !veried");
                return true;
            }
            if(root->getValues(NULL)->total_staked.toDouble()*0.7 < stake.toDouble())
            {
                if(!bp.heart_bit_sent_on_block_accepted_rsp)
                {
                    bp.heart_bit_sent_on_block_accepted_rsp=true;
                    do_heart_beat();

                }
            }
            last_access_time_hbZ=time(NULL);


        }
        break;
        case msgid::get_blocks_rsp:
        {
            msg::get_blocks_rsp r(in2);
            on_get_blocks_rsp(r);
            return true;
        }
        break;
        default:
            throw CommonError("unhandled22 p %s",msgName(p2));
        }
    }
    break;
    default:
        throw CommonError("unhandled11 p %s",msgName(p));

    }
    return true;
}
// #include "sql"
void Node::Service::dump_stats(const msg::publish_block&  pb)
{
    {

    }
}
void Node::Service::do_client_tx_report(const msg::publish_block &pb)
{
    MUTEX_INSPECTOR;
    for(auto &z:clientTxSubscriptions)
    {
        passEvent(new bcEvent::ClientTxSubscribeRSP(pb.getBuffer(),poppedFrontRoute(z.first)));
    }
}
void Node::Service::on_block_accepted_req(const msg::block_accepted_req& ba, const NODE_id& src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

    std::vector<BlsPublicKey> agg_pk;
    for(auto& z: ba.node_validators)
    {
        agg_pk.push_back(root->getNode(z,NULL)->bls_pk);
    }
    if(!ba.agg_sig.verify(agg_pk,blake2b_hash(ba.block_payload).container))
    {
        logNode("block aggsig not matched");
        return;
    }
    else
    {
        // logNode("block verified OK");
    }
    // prepared_block.block_accepted_req1=ba.getBuffer();
    msg::blockZ blk(ba.block_payload);

    msg::leader_certificate lc(ba.leader_certificateZ);
    msg::heart_beat hb(lc.payload_heart_beat);
    if(!root->verify_lider_certificate(lc))
    {
        logNode("leader cert not verified");
        return;
    }
    if(src_node!=hb.node_leader)
    {
        logNode("if(src_node!=hb.node_leader)");
        return;
    }

    db->write_batch(db_to_save_Z);
    db_to_save_Z.clear();

    msg::publish_block pb;
    pb.att_data=prepared_block.att_data;
    pb.block_accepted_req=ba.getBuffer();
    pb.epoch=prepared_block.epoch;
    do_client_tx_report(pb);
    dump_stats(pb);
    SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    dbs.exec("CREATE TABLE IF NOT EXISTS blocks ("
             "epoch INTEGER PRIMARY KEY, "
             "data BLOB NOT NULL)");

    SQLite::Statement insert(dbs, "INSERT INTO blocks (epoch, data) VALUES (?, ?)");
    insert.bind(1,pb.epoch.toString());
    insert.bind(2,iUtils->bin2hex(pb.getBuffer()));
    insert.exec();

    prev_block_hash=blk.new_root_hash1;
    // root=nullptr;
    root=getRoot(db.get());
    init_root(root);
    blocks.clear();
    msg::block_accepted_rsp br;
    br.new_root_hash=prev_block_hash;
    br.node_signer=this_node_name;
    br.sign(my_sk_bls);
    prepared_block.clear();
    blocks.clear();

    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

    msg::node_message_ed nm(br.getBuffer(),this_node_name,my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(route)));
    iUtils->getNow();

}
bool Node::Service::Msg(const bcEvent::Msg*e)
{
    MUTEX_INSPECTOR;

    inBuffer in(e->msg);


    auto p=in.get_PN();
    switch(p)
    {
    case msgid::node_message_ed:
    {
        MUTEX_INSPECTOR;
        msg::node_message_ed node_message_ed;
        node_message_ed.unpack(in);

        auto n=root->getNode(node_message_ed.src_node,NULL);
        if(!n.valid())
            throw CommonError("invalid node "+node_message_ed.src_node.container);
        if(!node_message_ed.verify(n->ed_pk))
        {
            throw CommonError("if(!node_message_ed.verify_ed_pk(n->ed_pk))");
        }
        inBuffer in2(node_message_ed.payload);
        auto p2=in2.get_PN();
        switch (p2)
        {
        case msgid::broadcast_tree:
        {
            MUTEX_INSPECTORS("broadcast_tree");
            msg::broadcast_tree bt;
            bt.unpack(in2);
            make_broadcast_message_to_tree(bt.payload,bt.tree,e->route);
            msg::broadcast_tree_ack bta;
            bta.hash_buf=blake2b_hash(e->msg);
            msg::node_message_ed nme(bta.getBuffer(),this_node_name,my_sk_ed);
            auto popped_route=poppedFrontRoute(e->route);
            passEvent(new bcEvent::MsgReply(nme.getBuffer(),popped_route));

            {
                MUTEX_INSPECTOR;
                inBuffer in_bt(bt.payload);
                auto p_bt=in_bt.get_PN();
                // logErr2("p_bt %d",p_bt);
                switch(p_bt)
                {
                case msgid::heart_beat:
                {
                    MUTEX_INSPECTOR;
                    //  logNode("case msgid::heart_beat:");
                    msg::heart_beat h;
                    h.unpack(in_bt);
                    on_heart_beat(h,bt.payload, e->route);
                }
                break;
                // default: break;
                case msgid::user_message_req:
                {
                    MUTEX_INSPECTOR;
                    msg::user_message_req um;
                    um.unpack(in_bt);
                    auto u=root->getUser(um.nick,NULL);
                    if(!u.valid())
                    {
                        logErr2("if(!u.valid()) %s %d",__FILE__,__LINE__);
                    }
                    if(!um.verify(u->pkbin))
                    {
                        logErr2("if(!um.verify_ed())");
                        return true;
                    }
                    if(u.valid())
                    {
                        if(u->nonce==um.nonce)
                            addToTransactionToPool(bt.payload);
                    }
                    // logErr2("case msgid::user_message_req:");
                }
                break;
                case msgid::node_message_ed:
                {
                    MUTEX_INSPECTOR;
                    msg::node_message_ed nm4(in_bt);
                    if(!nm4.verify(root->getNode(nm4.src_node,NULL)->ed_pk))
                    {
                        logNode("msg::node_message_ed verify failed");
                        return true;
                    }

                    inBuffer in4(nm4.payload);
                    int t4=in4.get_PN();
                    switch(t4)
                    {
                    case msgid::block_accepted_req:
                    {
                        if(state_Z!=State::NORMAL)
                            return true;
                        MUTEX_INSPECTORS("block_accepted");
                        // logNode("msgid::block_accepted");
                        msg::block_accepted_req ba(in4);
                        on_block_accepted_req(ba,nm4.src_node, e->route);
                        //////////////////////////

                    }
                    break;
                    case msgid::block_request:
                    {

                        MUTEX_INSPECTORS("block_request");

                        if(state_Z!=State::NORMAL)
                            return true;

                        // sendEvent(ServiceEnum::Executor,new bcEvent::Msg(nm4.payload,e->route));
                        // return true;
                        msg::block_request b(in4);
                        msg::leader_certificate lc(b.leader_cert);
                        if(!root->verify_lider_certificate(b.leader_cert))
                            throw CommonError("if(!verify_lider_certificate(b.leader_cert))");

                        msg::heart_beat hb(lc.payload_heart_beat);

                        if(hb.prev_block_hash!=prev_block_hash)
                        {
                            if(root->getValues(NULL)->epoch<hb.epoch)
                            {
                                //prev_block_hash=hb.prev_block_hash;
                                setBlockId(hb.prev_block_hash);
                                return true;
                            }
                            logNode("ERROR: block_request block %s, nextblock %s",hb.prev_block_hash.str().c_str(), prev_block_hash.str().c_str());

                        }
                        {

                            auto new_root_hash=execute_block(root,prev_block_hash, b.transaction_bodies,lc.nodes);
                            msg::blockZ block;
                            block.prev_root_hash=prev_block_hash;
                            block.new_root_hash1=new_root_hash;


                            block.attachment_hash.container=prepared_block.att_data.hash();

                            block.payload_heart_bit=lc.payload_heart_beat;

                            msg::block_response br;
                            br.node_validator=this_node_name;
                            br.payload_block=block.getBuffer();
                            br.sign(my_sk_bls);

                            msg::node_message_ed nn(br.getBuffer(),this_node_name,my_sk_ed);
                            passEvent(new bcEvent::MsgReply(nn.getBuffer(),poppedFrontRoute(e->route)));


                        }





                    }
                    break;
                    case msgid::request_for_transactions:
                    {
                        MUTEX_INSPECTOR;
                        msg::request_for_transactions rft(in4);
                        msg::leader_certificate lc(rft.payload_lc);
                        msg::heart_beat hb(lc.payload_heart_beat);

                        if(!root->verify_lider_certificate(lc))
                        {
                            logErr2("if(!verify_lider_certificate(rft.payload_lc,node_leader))");
                            return true;
                        }
                        if(nm4.src_node!=hb.node_leader)
                        {
                            logNode("messag src node != node leader %s %s",nm4.src_node.container.c_str(),hb.node_leader.container.c_str());
                            return true;
                        }
                        sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

                        last_access_time_hbZ=time(NULL);


                        if(hb.prev_block_hash!=prev_block_hash) /// todo непонятно как нода узнает достоверно, что предложенный hb.prev_block_hash валиден
                        {
                            logNode("root->getValues(NULL)->epoch<hb.epoch %s %s",root->getValues(NULL)->epoch.toString().c_str(),hb.epoch.toString().c_str());
                            if(root->getValues(NULL)->epoch<hb.epoch)
                            {
                                logNode("if(root->getValues(NULL)->epoch<hb.epoch)");
                                if(state_Z!=State::SYNCING)
                                {
                                    logNode("do_sync()");
                                    state_Z=State::SYNCING;
                                    last_leader_cert=rft.payload_lc;
                                    do_sync();
                                    return true;
                                }
                            }
                            else
                            {
                                logNode("invalid epoch, skipping");
                                return true;
                            }

                        }
                        msg::response_with_transactions rwt;
                        for(auto& z: transaction_pool_unverified)
                        {
                            rwt.trs.push_back(z.second);
                        }
                        transaction_pool_unverified.clear();
                        msg::node_message_ed nm(rwt.getBuffer(),this_node_name,my_sk_ed);
                        passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(e->route)));
                        return true;
                    }
                    break;
                    default:
                        logNode("unhandled pp4 %s %d",msgName(t4),t4);

                    }
                }
                break;
                default:
                    logErr2("unhandled msg dd3 %s",msgName(p_bt));
                    return true;
                    // break;
                }
            }
        }
        break;
        case msgid::get_blocks_req:
        {
            logNode("case msgid::get_blocks_req: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            msg::get_blocks_req gbr(in2);
            on_get_blocks_req(gbr,e->route);
            return true;

        }
        break;

        default:
            throw CommonError("unhabdled p2 %s",msgName(p2));
            break;
        }{

        }
    } break;
    default:
        throw CommonError("unhabdled p %s",msgName(p));
    }
    return true;
}
BLOCK_id Node::Service::execute_block(const REF_getter<root_data> &rt, const BLOCK_id & bl, const std::vector<TRANSACTION_body >& trs, const std::vector<NODE_id> &nodes_in_leader_cert)
{
    _feeCalcers feeCalcers;

    auto & bt=blocks[bl];
    if(bt.executed)
        throw CommonError("[%s] if(bt.executed) %s", this_node_name.container.c_str(),bl.str().c_str());
    bt.executed=true;
    t_params t(rt);
    // std::vector<std::string> errs;
    outBuffer o;
    t.instruction_reports.resize(trs.size());
    for(int ti=0; ti<trs.size(); ti++)
    {
        THASH_id th=blake2b_hash(trs[ti].container);
        msg::user_message_req ur(trs[ti]);
        REF_getter<fee_calcer> by=feeCalcers.get(ur.nick);
        auto u=rt->getUser(ur.nick,by);
        if(!u.valid())
            throw CommonError("if(!u.valid()) %s %d",__FILE__,__LINE__);
        if(!ur.verify(u->pkbin))
            throw CommonError("if(!ur.verify())");
        if(u->nonce != ur.nonce)
            throw CommonError("if(u->nonce != ur.nonce)");
        t.instruction_reports[ti].resize(ur.payload.size());
        execute_transaction(ti,t,ur.nick,ur.payload,by);
        BigInt one;
        one=1;
        u->nonce+=one;

        // fees.push_back(fee);
    }
    prepared_block.epoch=rt->getValues(NULL)->epoch;
    prepared_block.att_data.instruction_reports=t.instruction_reports;
    prepared_block.att_data.trs=trs;

    rt->getValues(NULL)->epoch+=1;

    auto new_root_hash=proceed_merkle_on_transaction_pool_hashers(rt);

    BigInt total_fees;
    for(auto& z: feeCalcers.calcers)
    {
        auto u=rt->getUser(z.first,NULL);
        if(!u.valid())
            throw CommonError("if(!u.valid()) 334455");
        if(u->balance < z.second->get_fee())
        {
            u->balance=0;
        }
        else {
            logErr2("balance deduct %s fee %s",u->balance.toString().c_str(), z.second->get_fee().toString().c_str());
            u->balance-=z.second->get_fee();
        }
        total_fees+=z.second->get_fee();
        prepared_block.att_data.fees[z.first]=z.second->get_fee();
    }
    BigInt total_rewards=(total_fees*9)/10;
    for(auto& n : nodes_in_leader_cert)
    {
        auto node=rt->getNode(n,NULL);
        if(!node.valid())
            throw CommonError("if(!node.valid()) 556677");
        auto owner=node->owner;
        auto u=rt->getUser(owner,NULL);
        if(!u.valid())
        {
            throw CommonError("if(!u.valid()) 778899");
            // u=root->addUser(upk,NULL);
        }
        BigInt amt=(total_rewards * node->total_stake) / rt->getValues(NULL)->total_staked;
        u->balance+=amt;
        if(n==this_node_name && amt>0)
            logNode("node %s rewarded %s grans",n.container.c_str(), amt.toString().c_str());
        prepared_block.att_data.rewards[n]=amt;
    }
    new_root_hash=proceed_merkle_on_transaction_pool_hashers(rt);
    feeCalcers.clear();
    return new_root_hash;

}
BLOCK_id Node::Service::proceed_merkle_on_transaction_pool_hashers(const REF_getter<root_data> &r)
{
    // _db_to_save db_to_save_L;
    auto &bt=blocks[prev_block_hash];

    r->calc_tree_hash(db_to_save_Z);

    auto root_buf=r->getBuffer();
    auto root_hash=blake2b_hash(root_buf);
    db_to_save_Z.add("#root#",root_buf);
    db_to_save_Z.add("#root_hash#",root_hash.container);
    BLOCK_id ret;
    ret.container=root_hash.container;
    return ret;


}


bool Node::Service::ClientMsg(const bcEvent::ClientMsg*e)
{


    MUTEX_INSPECTOR;
    inBuffer in(e->msg);

    auto p=in.get_PN();
    THASH_id hash;
    hash=blake2b_hash(e->msg);
    switch(p)
    {
    case msgid::user_request:
    {
        msg::user_request ur(in);


        inBuffer in2(ur.payload);
        auto p2=in2.get_PN();
        switch(p2)
        {
        case msgid::get_user_status_req:
        {
            MUTEX_INSPECTOR;
            msg::get_user_status_req rq(in2);
            logNode("get_user_status_req");
            std::optional<std::string> err;
            auto u=root->getUser(rq.nick,NULL);
            if(!u.valid())
                err="user not found "+rq.nick;

            msg::get_user_status_rsp r;
            if(!err)
            {
                r.nick=rq.nick;
                r.nonce=u->nonce;
                r.balance=u->balance;
            }
            logNode("passEvent(new bcEvent::ClientMsgReply(resp.getBuffer(), hash,poppedFrontRoute(e->route)));");
            passEvent(new bcEvent::ClientMsgReply(hash, r.getBuffer(),poppedFrontRoute(e->route)));

        }
        break;
        default:
            throw CommonError("unhandled msgid sdf %d",p);
        }


    }
    break;
    case msgid::user_message_req:
    {
        MUTEX_INSPECTOR;
        std::optional<std::string> err;
        sendEvent(ServiceEnum::TxValidator,new bcEvent::AddTx(e,this));
        msg::user_message_req um(in);
        auto u=root->getUser(um.nick,NULL);
        if(!u.valid())
            err="user not found "+um.nick;
        if(!err && !um.verify(u->pkbin))
        {
            err="verify failed";
            return true;

        }
        // logErr2("um.nonce %s",um.nonce.toString().c_str());
        if(!err && u->nonce!=um.nonce)
        {
            err="invalid_nonce "+ u->nonce.toString()+" != "+um.nonce.toString();
        }
        if(!err)
            addToTransactionToPool(e->msg);

        msg::transaction_added_rsp tr;
        tr.err=err.has_value();
        tr.err_str=err?*err:"transaction added to pool";
        tr.tx_hash=blake2b_hash(e->msg);
        // msg::node_message_ed nm(tr.getBuffer(),this_node_name,my_sk_ed);
        passEvent(new bcEvent::ClientMsgReply(hash, tr.getBuffer(),poppedFrontRoute(e->route)));
        return true;
    }
    break;
    default:
        throw CommonError("unhandled msgid e. ff %d",p);
    }

    return true;
}


void Node::Service::logNode(const char* fmt, ...)
{

    {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stdout,"%ld [Node] [%s] [%s] [%s] ", time(NULL), this_node_name.container.c_str(), prev_block_hash.str().c_str(), root->getValues(NULL)->epoch.toString().c_str());
        vfprintf(stdout,fmt, ap);
        fprintf(stdout,"\n");
        va_end(ap);

    }
}
bool Node::Service::ClientTxSubscribeREQ(const bcEvent::ClientTxSubscribeREQ* e)
{
    logNode("ClientTxSubscribeREQ from client");
    auto& s=clientTxSubscriptions[e->route];
    s.created_at=time(NULL);
    // s.backroute=e->route;
    return true;
}
