#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <time.h>
#include <map>
#include "broadcasterTreeService.h"
#include "tools_mt.h"
#include "tree.h"
#include "events_broadcasterTreeService.hpp"
#include "version_mega.h"
#include "tr_exec.h"
#include "CDatabase.h"
#include "s_ed.h"
#include "QUORUM.h"
#include <SQLiteCpp/Database.h>
#include "CDatabase.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "init_root.h"



bool BroadcasterTree::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;

    return true;
}

bool BroadcasterTree::Service::on_timer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool BroadcasterTree::Service::on_alarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    switch(e->tid)
    {
    case TIMER_BROADCAST_ACK_TIMEDOUT:
    {
        MUTEX_INSPECTOR;
        TIMER_BROADCAST_ACK_TIMEDOUT_cookie *c=dynamic_cast<TIMER_BROADCAST_ACK_TIMEDOUT_cookie*>(e->cookie.get());
        if(!c) throw CommonError("if(!c) 1222447");

        logErr2("TIMER_BROADCAST_ACK_TIMEDOUT %s",c->dstName_.container.c_str());

        make_broadcast_message_to_tree(c->dstService, c->msg,c->tree, c->route);

        return true;

    }
    break;

    }
    return false;

    return false;
}


bool BroadcasterTree::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        switch(ID)
        {
        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot*)e.get());
        case bcEventEnum::BroadcastMessage:
            return BroadcastMessage((const bcEvent::BroadcastMessage*)e.get());
        // case bcEventEnum::GetTransactions:
        //     return GetTransactions((const bcEvent::GetTransactions*)e.get());
        // case bcEventEnum::ClientMsg:
        //     return ClientMsg((const bcEvent::ClientMsg*)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit*)e.get());
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer*)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm*)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService*)e.get());
        case bcEventEnum::MsgReply:
            return MsgReply(static_cast<const bcEvent::MsgReply*>(e.get()),false);
        case bcEventEnum::Msg:
            return Msg(static_cast<const bcEvent::Msg*>(e.get()),false);

        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor*ev=static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get());
            auto &IDA=ev->e->id;

            switch(IDA)
            {
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply*>(ev->e.get()),true);
            case bcEventEnum::Msg:
                return Msg(static_cast<const bcEvent::Msg*>(ev->e.get()),true);

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
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply*>(ev->e.get()),true);
            case bcEventEnum::Msg:
                return Msg(static_cast<const bcEvent::Msg*>(ev->e.get()),true);

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
        logErr2("BroadcasterTree std::exception  %s",e.what());
    }
    XPASS;
    return false;
}
#include <regex>

BroadcasterTree::Service::~Service()
{
}


BroadcasterTree::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
}

bool BroadcasterTree::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf=e;
    if(!root.valid())
        root=getRoot(conf->db.get());

    init_root(root);
    return true;
}
bool BroadcasterTree::Service::GetTransactions(const bcEvent::GetTransactions*e)
{
    msg::response_with_transactions rwt;
    for(auto& z: transaction_pool_verified)
    {
        rwt.trs.push_back(z.second);
    }
    transaction_pool_verified.clear();
    msg::node_message_ed nm(rwt.getBuffer(),conf->this_node_name,conf->my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(e->route)));

    return true;
}
bool BroadcasterTree::Service::InvalidateRoot(const bcEvent::InvalidateRoot*e)
{
    root=getRoot(conf->db.get());
    init_root(root);
    return true;
}
bool BroadcasterTree::Service::BroadcastMessage(const bcEvent::BroadcastMessage*e)
{
    MUTEX_INSPECTOR;
    std::map<NODE_id,BroadcasterTree::Node> nodes;

    auto ks=root->getNodesNames(NULL);
    for(auto &nd: ks)
    {
        auto nn=root->getNode(nd,NULL);
        if(nn->disabled_manual || nn->disabled_offline)
            continue;
        BroadcasterTree::Node n;
        n.name=nn->name;
        n.stake=nn->total_stake;
        n.ip=nn->ip;
        nodes[nn->name]=n;
    }
    if(nodes.size()==0)
        return true;
    BroadcasterTree::TreeNode root = BroadcasterTree::buildTree(nodes, conf->this_node_name);

    make_broadcast_message_to_tree(ServiceEnum::BroadcasterTree, e->msg,root, e->route);
    return true;    
}
// void BroadcasterTree::Service::make_broadcast_message(const std::string & msg)
// {
// }

// void BroadcasterTree::Service::make_broadcast_message(const std::vector<uint8_t> & msg)
// {
//     MUTEX_INSPECTOR;
//     make_broadcast_message({(char*)msg.data(),msg.size()});
// }
void BroadcasterTree::Service::make_broadcast_message_to_tree(SERVICE_id dstService, const std::string & msg, const BroadcasterTree::TreeNode& root, const route_t& route)
{
    MUTEX_INSPECTOR;
    auto &ch=root.children;
    // logErr2("ch.size %d",ch.size());
    for(auto it=ch.begin(); it!=ch.end(); it++)
    {
        MUTEX_INSPECTOR;
        // logErr2("send to %s",it->node.name.c_str());
        msg::broadcast_tree bt;
        bt.tree=*it;
        bt.payload=msg;

        msg::node_message_ed nm(bt.getBuffer(),conf->this_node_name,conf->my_sk_ed);
        auto sb=nm.getBuffer();
        sendEvent(it->node.ip, ServiceEnum::BroadcasterTree, new bcEvent::Msg(sb,route));


        REF_getter<TIMER_BROADCAST_ACK_TIMEDOUT_cookie> cc=new TIMER_BROADCAST_ACK_TIMEDOUT_cookie;
        cc->dstService=dstService;
        cc->dstName_=it->node.name;
        cc->msg=msg;
        cc->tree=(*it);
        cc->route=route;
        sendEvent(ServiceEnum::Timer,new timerEvent::SetAlarm(TIMER_BROADCAST_ACK_TIMEDOUT,
                  toRef(blake2b_hash(sb).container), cc.get(), BROADCAST_ACK_TIMEDOUT_SEC, this));

    }

}

bool BroadcasterTree::Service::Msg(const bcEvent::Msg*e, bool fromNetwork)
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
            throw CommonError("invalid node BBB "+node_message_ed.src_node.container);
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
            logNode("Msg: broadcast_tree from %s",node_message_ed.src_node.container.c_str());
            msg::broadcast_tree bt;
            bt.unpack(in2);
            make_broadcast_message_to_tree(bt.dst_service, bt.payload,bt.tree,e->route);
            msg::broadcast_tree_ack bta;
            bta.hash_buf=blake2b_hash(e->msg);
            msg::node_message_ed nme(bta.getBuffer(),conf->this_node_name,conf->my_sk_ed);
            passEvent(new bcEvent::MsgReply(nme.getBuffer(),poppedFrontRoute(e->route)));
        
#ifdef KALL
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
                    throw CommonError("case msgid::user_message_req: not implemented");
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
                        sendEvent(ServiceEnum::TxValidator,new bcEvent::GetTransactions(e->route));
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
#endif
        }
        break;

        #ifdef KALL
        case msgid::get_blocks_req:
        {
            logNode("case msgid::get_blocks_req: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            msg::get_blocks_req gbr(in2);
            on_get_blocks_req(gbr,e->route);
            return true;

        }
        break;
        #endif

        default:
            sendEvent(ServiceEnum::Node,new bcEvent::Msg(node_message_ed.payload,e->route));

            // throw CommonError("unhabdled p22222 %s",msgName(p2));
            break;
        }{

        }
    } break;
    default:
        throw CommonError("unhabdled p %s",msgName(p));
    }
    return true;
}

bool BroadcasterTree::Service::MsgReply(const bcEvent::MsgReply* e, bool fromNetwork)
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
            throw CommonError("invalid node AAA "+node_message_ed.src_node.container);
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
#ifdef KALL
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
                transaction_pool_of_leader.insert({h,z});
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

            blst_cpp::AggregateSignature agg_sig;
            std::vector<blst_cpp::PublicKey> agg_pk;
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
#endif
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
#ifdef KALL
bool BroadcasterTree::Service::ClientMsg(const bcEvent::ClientMsg*e)
{


    MUTEX_INSPECTOR;
    inBuffer in(e->msg);

    auto p=in.get_PN();
    THASH_id hash;
    hash=blake2b_hash(e->msg);
    switch(p)
    {
    case msgid::user_message_req:
    {
        MUTEX_INSPECTOR;
        std::optional<std::string> err;
        // sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::AddTx(e,this));
        msg::user_message_req um(in);
        if(!err && !um.verify())
        {
            err="verify failed";
            // return true;

        }
        BigInt nonce=0;
        // logErr2("getUser %s",base62::encode(um.address_pk_ed).c_str());
        if(!err)
        {
            auto u=root->getUser(um.address_pk_ed,NULL);
            if(u.valid())
            {
                nonce=u->nonce;
            }
        }

        // logErr2("um.nonce %s",um.nonce.toString().c_str());
        if(!err && nonce!=um.nonce)
        {
            err="invalid_nonce "+ nonce.toString()+" != "+um.nonce.toString();
        }
        if(!err)
        {
            THASH_id h=blake2b_hash(e->msg);
            TRANSACTION_body t;
            t.container=e->msg;
            transaction_pool_verified.insert({h,t});
        }
            // addToTransactionToPool(e->msg);
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
#endif
void BroadcasterTree::Service::logNode(const char* fmt, ...)
{

    {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stdout,"%ld [BroadcasterTree] [%s] ", time(NULL), 
        conf->this_node_name.container.c_str());
        vfprintf(stdout,fmt, ap);
        fprintf(stdout,"\n");
        va_end(ap);

    }
}

void registerBroadcasterTreeService(const char* pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::BroadcasterTree,"BroadcasterTree",getEvents_broadcasterTreeService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::BroadcasterTree,BroadcasterTree::Service::construct,"BroadcasterTree");
        regEvents_broadcasterTreeService();
    }
    XPASS;
}




