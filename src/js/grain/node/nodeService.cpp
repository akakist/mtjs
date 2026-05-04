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


    my_sk_bls.deserializeBase62Str(getenv2(my_sk_bls_env_key));


    my_sk_ed=base62::decode(getenv2(my_sk_ed_env_key));
    logErr2("ServiceInit nodename %s",this_node_name.container.c_str());
    sendEvent(ServiceEnum::BlockValidator,new bcEvent::ServiceInit(my_sk_bls,my_sk_ed,this_node_name,db, this));
    sendEvent(ServiceEnum::TxValidator,new bcEvent::ServiceInit(my_sk_bls,my_sk_ed,this_node_name,db, this));
    sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::ServiceInit(my_sk_bls,my_sk_ed,this_node_name,db, this));
    sendEvent(ServiceEnum::GrainReader,new bcEvent::ServiceInit(my_sk_bls,my_sk_ed,this_node_name,db, this));
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

    msgFactory.registerMsg(msgid::HeartBeatREQ,MsgEvent::HeartBeatREQ::construct);
    msgFactory.registerMsg(msgid::HeartBeatRSP,MsgEvent::HeartBeatRSP::construct);
    msgFactory.registerMsg(msgid::GetTransactionREQ,MsgEvent::GetTransactionREQ::construct);
    msgFactory.registerMsg(msgid::GetTransactionRSP,MsgEvent::GetTransactionRSP::construct);
    msgFactory.registerMsg(msgid::ValidateBlockREQ,MsgEvent::ValidateBlockREQ::construct);
    msgFactory.registerMsg(msgid::ValidateBlockRSP,MsgEvent::ValidateBlockRSP::construct);
    msgFactory.registerMsg(msgid::BlockAcceptedREQ,MsgEvent::BlockAcceptedREQ::construct);
    msgFactory.registerMsg(msgid::BlockAcceptedRSP,MsgEvent::BlockAcceptedRSP::construct);
    msgFactory.registerMsg(msgid::GetSavedBlocksREQ,MsgEvent::GetSavedBlocksREQ::construct);
    msgFactory.registerMsg(msgid::GetSavedBlocksRSP,MsgEvent::GetSavedBlocksRSP::construct);
    

    return true;
}

void Node::Service::collectTransactions()
{
    MUTEX_INSPECTOR;
    std::map<std::string, std::set<THASH_id>> cnt;
    std::set<THASH_id> rm;
    for(auto& z: transaction_pool_of_leader)
    {
        msg::user_message_req ur(z.second);
        cnt[ur.address_pk_ed].insert(z.first);
        BigInt nonce=0;
        auto u=root->getUser(ur.address_pk_ed,NULL);
        if(u.valid())
        {
            nonce=u->nonce;
        }
            // throw CommonError("if(!u.valid()) %s %d",__FILE__,__LINE__);
        if(nonce!=ur.nonce)
        {
            // logErr2("tr: %s nonce %s %s",u->nonce.toString().c_str(), ur.nonce.toString().c_str());
            logErr2("tr: %s declined due invalid nonce",base62::encode(z.first.str()).c_str());
            rm.insert(z.first);
        }

    }
    for(auto& z: rm)
    {
        transaction_pool_of_leader.erase(z);
    }
    rm.clear();

    for(auto& z: cnt)
    {
        if(z.second.size()>1)
        {
            for(auto &k: z.second)
            {
                logErr2("tr: %s declined due multiple transaction from one sender",base62::encode(z.first).c_str());
                rm.insert(k);
            }
        }
    }
    for(auto& z: rm)
    {
        transaction_pool_of_leader.erase(z);
    }
    rm.clear();
}
// void 
void Node::Service::do_start_block()
{
    MUTEX_INSPECTOR;
    if(transaction_pool_of_leader.empty())
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
            REF_getter<MsgEvent::ValidateBlockREQ> b= new MsgEvent::ValidateBlockREQ();
            // msg::block_request b;
            b->leader_cert=li.leader_cert;

            auto & bt=blocks[prev_block_hash];

            collectTransactions();

            for(auto& z: transaction_pool_of_leader)
                b->transaction_bodies.push_back(z.second);
            transaction_pool_of_leader.clear();

   
            outBuffer ob;
            b->pack(ob);
            msg::node_message_ed nm(ob.asString()->container,this_node_name,my_sk_ed);


            sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(),ListenerBase::serviceId));
            // make_broadcast_message(nm.getBuffer());
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

    }
    return false;
}


bool Node::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    // if(e->route.size())
    // {
    //     passEvent(e);
    //     return true;
    // }
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
        case bcEventEnum::ClientMsgReply:
            passEvent(e);
            return true;
        case bcEventEnum::Msg:
            return Msg(static_cast<const bcEvent::Msg*>(e.get()),false);
        case bcEventEnum::MsgReply:
            return MsgReply(static_cast<const bcEvent::MsgReply*>(e.get()),false);
        case httpEventEnum::RequestIncoming:
            return RequestIncoming(static_cast<const httpEvent::RequestIncoming*>(e.get()));
        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor*ev=static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get());
            auto &IDA=ev->e->id;

            switch(IDA)
            {
            case bcEventEnum::Msg:
                return Msg(static_cast<const bcEvent::Msg*>(ev->e.get()),true);
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply*>(ev->e.get()),true);
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
            case bcEventEnum::Msg:
                return Msg(static_cast<const bcEvent::Msg*>(ev->e.get()),true);
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply*>(ev->e.get()),true);

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
    MUTEX_INSPECTOR;
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
    MUTEX_INSPECTOR;
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

// void Node::Service::on_blockResponse(const msg::block_response& br)
bool Node::Service::ValidateBlockRSP(const MsgEvent::ValidateBlockRSP* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    if(!r->verify(root->getNode(r->node_validator,NULL)->bls_pk))
    {
        logErr2("block response not validated");
        return true;
    }

    // msg::blockZ bl(r->payload_block);
    if(r->payload_block->prev_root_hash!=prev_block_hash)
    {
        logErr2("if(bl.prev_root_hash!=prev_block_hash)");
        return true;
    }
    // if(bl.)
    auto & bt=blocks[prev_block_hash];
    if(bt.block_accepted_sent)
        return true;
    // auto & bh=bt[bl.root_hash];
    bt.responses.push_back(r);
    bt.stake+=root->getNode(r->node_validator,NULL)->total_stake;
    // logNode("Block staked %lf",bt.stake.toDouble());
    if(bt.stake.toDouble() > root->getValues(NULL)->total_staked.toDouble()*QUORUM)
    {
        logNode("Block stake finalized");
        REF_getter<MsgEvent::BlockAcceptedREQ>  ba=new MsgEvent::BlockAcceptedREQ();
        if(!bt.block_payload.valid())
        {
            bt.block_payload=r->payload_block;
        }
        else if(bt.block_payload->getBuffer()!=r->payload_block->getBuffer())
            throw("else if(bh.block_payload!=r->payload_block)");

        ba->block_payload=bt.block_payload;
        if(!bt.block_payload.valid())
            throw CommonError("if(!bt.block_payload.valid())");
        std::vector<blst_cpp::PublicKey> agg_pk;

        ba->leader_certificateZ=heart_beat_store.leader_info[heart_beat_store.node_leader].leader_cert;
        for(auto& z: bt.responses)
        {
            auto n=root->getNode(z->node_validator,NULL);
            agg_pk.push_back(n->bls_pk);
            ba->agg_sig.add(z->sig);
            ba->node_validators.push_back(z->node_validator);
        }
        if(ba->agg_sig.verify(agg_pk,blake2b_hash(ba->block_payload->getBuffer()).container))
        {
            // logErr2("compose block_accepted test verified OK !!!!!!!!!!!!!!!!!!!!!");
        }
        else
            logErr2("block_accepted verified FAIL !!!!!!!!!!!!!!!!!!!!!");

            // outBuffer ba_buf;
            // ba->pack(ba_buf);
        msg::node_message_ed nm(ba->getBuffer(),this_node_name,my_sk_ed);
        // make_broadcast_message(nm.getBuffer());
        sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(),ListenerBase::serviceId));

        bt.block_accepted_sent=true;
    }

    return true;
}
void Node::Service::do_request_for_transactions(const Node::heart_beat_node_info& li)
{
    MUTEX_INSPECTOR;
    REF_getter<MsgEvent::GetTransactionREQ> rt=new MsgEvent::GetTransactionREQ();
    rt->payload_lc=li.leader_cert;
    msg::node_message_ed nm(rt->getBuffer(),this_node_name,my_sk_ed);
    sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(),ListenerBase::serviceId));

}
bool Node::Service::GetTransactionRSP(const MsgEvent::GetTransactionRSP* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
            for(auto& z: r->trs)
            {
                THASH_id h=blake2b_hash(z.container);
                transaction_pool_of_leader.insert({h,z});
            }
            auto &hbs=heart_beat_store;
            auto &li=hbs.leader_info[hbs.node_leader];
            li.transaction_responders.insert(src_node);
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
bool Node::Service::BlockAcceptedRSP(const MsgEvent::BlockAcceptedRSP* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
            if(!r->verify(root->getNode(r->node_signer,NULL)->bls_pk))
            {
                logErr2("block_accepted_rsp: verify failed");
                return true;
            }
            auto &bp=blocks[prev_block_hash];
            bp.acceptors.insert({r->node_signer,r});

            blst_cpp::AggregateSignature agg_sig;
            std::vector<blst_cpp::PublicKey> agg_pk;
            BigInt stake;
            stake=0;
            for(auto &z:bp.acceptors)
            {
                agg_sig.add(z.second->sig_bls);
                auto n=root->getNode(z.first,NULL);
                if(!n.valid())
                    throw CommonError("if(!n.valid())");

                agg_pk.push_back(n->bls_pk);
                stake+=n->total_stake;
            }
            if(!agg_sig.verify(agg_pk,r->new_root_hash.container))
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



    return true;
}
bool Node::Service::GetSavedBlocksRSP(const MsgEvent::GetSavedBlocksRSP* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    for(auto& z: r->blocks_Z)
    {
        // auto & epoch=z.first
        // msg::publish_block pb(z.second);
        // msg::block_accepted_req ba(pb.block_accepted_req);
        // msg::leader_certificate lc(ba.leader_certificateZ);
        // msg::blockZ bl(ba.block_payload);
        // msg::heart_beat hb(bl.payload_heart_bit);
        if(z.second->epoch!=z.first)
            throw CommonError("if(hb.epoch!=z.first)");
        if(z.second->epoch!=root->getValues(NULL)->epoch)
            throw CommonError("if(hb.epoch!=root->getValues(NULL)->epoch) %s %s",z.second->epoch.toString().c_str(), root->getValues(NULL)->epoch.toString().c_str()   );

        std::vector<blst_cpp::PublicKey> agg_pk;
        for(auto& k: z.second->block_accepted_req->node_validators)
        {
            auto n=root->getNode(k,NULL);
            agg_pk.push_back(n->bls_pk);
        }
        if(!z.second->block_accepted_req->agg_sig.verify(agg_pk,blake2b_hash(z.second->block_accepted_req->block_payload->getBuffer()).container))
        {
            throw CommonError("on_get_blocks_rsp: !ba.agg_sig.verify");
        }
        logNode("on_get_blocks_rsp: block verified OK");
        // logNode("on_get_blocks_rsp: my epoch %s hb.epoch %s",root->getValues(NULL)->epoch.toString().c_str(), hb.epoch.toString().c_str() );

        SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        dbs.exec("CREATE TABLE IF NOT EXISTS blocks ("
                 "epoch INTEGER PRIMARY KEY, "
                 "data BLOB NOT NULL)");

        SQLite::Statement insert(dbs, "INSERT INTO blocks (epoch, data) VALUES (?, ?)");
        insert.bind(1,z.second->epoch.toString());
        insert.bind(2,z.second->getBuffer());
        insert.exec();

        auto new_root_hash=execute_block(root, prev_block_hash, z.second->att_data.trs,z.second->block_accepted_req->leader_certificateZ->nodes);
        if(new_root_hash==z.second->block_accepted_req->block_payload->new_root_hash1)
        {
            logNode("on_get_blocks_rsp: block executed OK on epoch %s",z.second->epoch.toString().c_str());
            root->getValues(NULL)->epoch=z.second->epoch+1;
            db->write_batch(db_to_save_Z);
            db_to_save_Z.clear();

            prev_block_hash=new_root_hash;

        }
        else
        {
            throw CommonError("if(new_root_hash!=bl.new_root_hash1)");
        }




    }
    if(r->lastEpoch > root->getValues(NULL)->epoch)
    {
        logNode("do_sync again: r.lastEpoch %s > root->getValues(NULL)->epoch %s",r->lastEpoch.toString().c_str(), root->getValues(NULL)->epoch.toString().c_str() );
        do_sync();
        return true;
    }
    state_Z=State::NORMAL;
    logNode("State::NORMAL");

    return true;
}
bool Node::Service::GetSavedBlocksREQ(const MsgEvent::GetSavedBlocksREQ* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READONLY);

    SQLite::Statement query(dbs, "SELECT epoch, data FROM blocks WHERE epoch>=? order by epoch limit 100");
    query.bind(1,r->myEpoch.toString());
    REF_getter<MsgEvent::GetSavedBlocksRSP>  ret=new MsgEvent::GetSavedBlocksRSP();
    while (query.executeStep())
    {
        // BigInt ep;
        BigInt ep = query.getColumn(0).getInt();
        std::string data = query.getColumn(1).getString();
        inBuffer in(data);
        REF_getter<MsgEvent::BlockDBStore> bds=new MsgEvent::BlockDBStore();
        bds->unpack2(in);
        ret->blocks_Z.push_back({ep,bds});
    }
    ret->lastEpoch=root->getValues(NULL)->epoch;
    msg::node_message_ed nm(ret->getBuffer(),this_node_name,my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(route)));

    return true;
}


bool Node::Service::MsgReply(const bcEvent::MsgReply* e, bool fromNetwork)
{
        MUTEX_INSPECTOR;

    if(e->route.size())
    {
        passEvent(e);
        return true;
    }
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
            throw CommonError("invalid node AAA "+node_message_ed.src_node.container);
        if(!node_message_ed.verify(n->ed_pk))
        {
            throw CommonError("if(!node_message_ed.verify_ed_pk(n->ed_pk))");
        }
        inBuffer in2(node_message_ed.payload);
        auto p2=in2.get_PN();

        REF_getter<MsgEvent::Base> m=msgFactory.create(p2);
        m->unpack(in2);
        switch(m->type)
        {
            case msgid::HeartBeatRSP:
                return HeartBeatRSP(dynamic_cast<const MsgEvent::HeartBeatRSP*>(m.get()),node_message_ed.src_node, e->route);
            case msgid::GetTransactionRSP:
                return GetTransactionRSP(dynamic_cast<const MsgEvent::GetTransactionRSP*>(m.get()),node_message_ed.src_node, e->route);
            case msgid::ValidateBlockRSP:
                return ValidateBlockRSP(dynamic_cast<const MsgEvent::ValidateBlockRSP*>(m.get()),node_message_ed.src_node, e->route);   
            case msgid::BlockAcceptedRSP:
                return BlockAcceptedRSP(dynamic_cast<const MsgEvent::BlockAcceptedRSP*>(m.get()),node_message_ed.src_node, e->route);   
            case msgid::GetSavedBlocksRSP:
                return GetSavedBlocksRSP(dynamic_cast<const MsgEvent::GetSavedBlocksRSP*>(m.get()),node_message_ed.src_node, e->route);   
            default:
                throw CommonError("unhandled22 p020 %s",msgName(p2));
                break;
        }
    }
    break;
    default:
        throw CommonError("unhandled11 p %s",msgName(p));

    }
    return true;
}
// #include "sql"
bool Node::Service::BlockAcceptedREQ(const MsgEvent::BlockAcceptedREQ* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;

    if(state_Z!=State::NORMAL)
    return true;

    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

    std::vector<blst_cpp::PublicKey> agg_pk;
    for(auto& z: r->node_validators)
    {
        agg_pk.push_back(root->getNode(z,NULL)->bls_pk);
    }
    if(!r->agg_sig.verify(agg_pk,blake2b_hash(r->block_payload->getBuffer()).container))
    {
        logNode("block aggsig not matched");
        return true;
    }
    else
    {
        // logNode("block verified OK");
    }
    if(!root->verify_lider_certificate(r->leader_certificateZ))
    {
        logNode("leader cert not verified");
        return true;
    }
    if(src_node!=r->leader_certificateZ->heart_beat->node_leader)
    {
        logNode("if(src_node!=hb.node_leader)");
        return true;
    }

    db->write_batch(db_to_save_Z);
    db_to_save_Z.clear();

    REF_getter<MsgEvent::BlockDBStore> pb=new MsgEvent::BlockDBStore();
    // msg::publish_block pb;
    pb->att_data=prepared_block.att_data;
    outBuffer o;
    r->pack(o);
    pb->block_accepted_req=r;
    pb->epoch=prepared_block.epoch;
    sendEvent(ServiceEnum::BlockStreamer,new bcEvent::StreamBlock(pb->getBuffer(),this));
    SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    dbs.exec("CREATE TABLE IF NOT EXISTS blocks ("
             "epoch INTEGER PRIMARY KEY, "
             "data BLOB NOT NULL)");

    SQLite::Statement insert(dbs, "INSERT INTO blocks (epoch, data) VALUES (?, ?)");
    insert.bind(1,pb->epoch.toString());
    insert.bind(2,pb->getBuffer());
    insert.exec();

    prev_block_hash=r->block_payload->new_root_hash1;
    // root=nullptr;
    root=getRoot(db.get());
    init_root(root);
    blocks.clear();
    sendEvent(ServiceEnum::TxValidator,new bcEvent::InvalidateRoot(this));
    sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::InvalidateRoot(this));
    sendEvent(ServiceEnum::GrainReader,new bcEvent::InvalidateRoot(this));
    REF_getter<MsgEvent::BlockAcceptedRSP> br=new MsgEvent::BlockAcceptedRSP();
    br->new_root_hash=prev_block_hash;
    br->node_signer=this_node_name;
    br->sign(my_sk_bls);
    prepared_block.clear();
    blocks.clear();

    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

    msg::node_message_ed nm(br->getBuffer(),this_node_name,my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(route)));
    iUtils->getNow();
    return true;

}
bool Node::Service::GetTransactionREQ(const MsgEvent::GetTransactionREQ* r, const NODE_id & src_node, const route_t& route)
{
            MUTEX_INSPECTOR;

            if(!root->verify_lider_certificate(r->payload_lc))
            {
                logErr2("if(!verify_lider_certificate(rft.payload_lc,node_leader))");
                return true;
            }
            if(src_node!=r->payload_lc->heart_beat->node_leader)
            {
                logNode("messag src node != node leader %s %s",src_node.container.c_str(),r->payload_lc->heart_beat->node_leader.container.c_str());
                return true;
            }
            sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

            last_access_time_hbZ=time(NULL);


            if(r->payload_lc->heart_beat->prev_block_hash!=prev_block_hash) /// todo непонятно как нода узнает достоверно, что предложенный hb.prev_block_hash валиден
            {
                logNode("root->getValues(NULL)->epoch<hb.epoch %s %s",root->getValues(NULL)->epoch.toString().c_str(),r->payload_lc->heart_beat->epoch.toString().c_str());
                if(root->getValues(NULL)->epoch<r->payload_lc->heart_beat->epoch)
                {
                    logNode("if(root->getValues(NULL)->epoch<hb.epoch)");
                    if(state_Z!=State::SYNCING)
                    {
                        logNode("do_sync()");
                        state_Z=State::SYNCING;
                        last_leader_cert=r->payload_lc;
                        do_sync();
                    }
                }
                else
                {
                    logNode("invalid epoch, skipping");
                    return true;
                }

            }
            sendEvent(ServiceEnum::TxValidator,new bcEvent::GetTransactions(route));
            return true;
}

bool Node::Service::ValidateBlockREQ(const MsgEvent::ValidateBlockREQ* r, const NODE_id & src_node, const route_t& route)
{

            MUTEX_INSPECTORS("ValidateBlockREQ");

            if(state_Z!=State::NORMAL)
                return true;

            if(!root->verify_lider_certificate(r->leader_cert))
                throw CommonError("if(!verify_lider_certificate(b.leader_cert))");


            if(r->leader_cert->heart_beat->prev_block_hash!=prev_block_hash)
            {
                if(root->getValues(NULL)->epoch<r->leader_cert->heart_beat->epoch)
                {
                    setBlockId(r->leader_cert->heart_beat->prev_block_hash);
                    return true;
                }
                logNode("ERROR: ValidateBlock block %s, nextblock %s",r->leader_cert->heart_beat->prev_block_hash.str().c_str(), prev_block_hash.str().c_str());

            }
            {

                auto new_root_hash=execute_block(root,prev_block_hash, r->transaction_bodies,r->leader_cert->nodes);
                REF_getter<MsgEvent::BlockInfo> block=new MsgEvent::BlockInfo();
                block->prev_root_hash=prev_block_hash;
                block->new_root_hash1=new_root_hash;


                block->attachment_hash.container=prepared_block.att_data.hash();

                block->payload_heart_beat=r->leader_cert->heart_beat;

                REF_getter <MsgEvent::ValidateBlockRSP> rsp=new MsgEvent::ValidateBlockRSP();
                // msg::block_response br;
                rsp->node_validator=this_node_name;
                rsp->payload_block=block;
                rsp->sign(my_sk_bls);

                msg::node_message_ed nn(rsp->getBuffer(),this_node_name,my_sk_ed);
                passEvent(new bcEvent::MsgReply(nn.getBuffer(),poppedFrontRoute(route)));


            }
            return true;

}

bool Node::Service::Msg(const bcEvent::Msg*e, bool fromNetwork)
{

    MUTEX_INSPECTOR;

    inBuffer in(e->msg);

    auto p=in.get_PN();
    switch(p)
    {
    case msgid::node_message_ed:
    {
        MUTEX_INSPECTOR;
        // logNode("case msgid::node_message_ed:");
        msg::node_message_ed node_message_ed;
        node_message_ed.unpack(in);

        auto n=root->getNode(node_message_ed.src_node,NULL);
        if(!n.valid())
            throw CommonError("invalid node BBB "+node_message_ed.src_node.container);
        if(!node_message_ed.verify(n->ed_pk))
        {
            throw CommonError("if(!node_message_ed.verify_ed_pk(n->ed_pk))");
        }
        inBuffer in2(node_message_ed.payload);

        auto p2=in2.get_PN();
        REF_getter<MsgEvent::Base> msg=msgFactory.create(p2);
        msg->unpack(in2);

        switch(msg->type)
        {
            case msgid::GetTransactionREQ:
                return GetTransactionREQ(dynamic_cast<const MsgEvent::GetTransactionREQ*>(msg.get()),node_message_ed.src_node, e->route);
            case msgid::HeartBeatREQ:
                return HeartBeatREQ(dynamic_cast<const MsgEvent::HeartBeatREQ*>(msg.get()),node_message_ed.payload, e->route);
            case msgid::ValidateBlockREQ:
                return ValidateBlockREQ(dynamic_cast<const MsgEvent::ValidateBlockREQ*>(msg.get()),node_message_ed.src_node, e->route);
            case msgid::BlockAcceptedREQ:
                return BlockAcceptedREQ(dynamic_cast<const MsgEvent::BlockAcceptedREQ*>(msg.get()),node_message_ed.src_node, e->route);
            case msgid::GetSavedBlocksREQ:
                return GetSavedBlocksREQ(dynamic_cast<const MsgEvent::GetSavedBlocksREQ*>(msg.get()),node_message_ed.src_node, e->route);

                default: throw CommonError("unjandled msgEvent %s",msgName(msg->type));
        }
    } break;
    default:
        throw CommonError("unhabdled Zp11 %s",msgName(p));
    }
    return true;
}
BLOCK_id Node::Service::execute_block(const REF_getter<root_data> &rt, const BLOCK_id & bl, const std::vector<TRANSACTION_body >& trs, const std::vector<NODE_id> &nodes_in_leader_cert)
{
    MUTEX_INSPECTOR;
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
        REF_getter<fee_calcer> by=feeCalcers.get(ur.address_pk_ed);
        // if(!u.valid())
        //     throw CommonError("if(!u.valid()) %s %d",__FILE__,__LINE__);
        if(!ur.verify())
            throw CommonError("if(!ur.verify())");
        BigInt nonce=0;
        auto u=rt->getUser(ur.address_pk_ed,by);
        if(u.valid())
        {
            nonce=u->nonce;
        }
        if(nonce != ur.nonce)
            throw CommonError("if(u->nonce != ur.nonce)");
        t.instruction_reports[ti].resize(ur.payload.size());
        execute_transaction(ti,t,ur.address_pk_ed,ur.payload,by);
        BigInt one;
        one=1;
        if(!u.valid())
            u=rt->addUser(ur.address_pk_ed,NULL);
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
        auto owner=node->owner_ed_pk;
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
