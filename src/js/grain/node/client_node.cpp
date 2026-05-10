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


bool Node::Service::GetSavedBlocksREQ(const MsgEvt::GetSavedBlocksREQ* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READONLY);

    SQLite::Statement query(dbs, "SELECT epoch, data FROM blocks WHERE epoch>=? order by epoch limit 100");
    query.bind(1,r->myEpoch.toString());
    REF_getter<MsgEvt::GetSavedBlocksRSP>  ret=new MsgEvt::GetSavedBlocksRSP();
    while (query.executeStep())
    {
        // BigInt ep;
        BigInt ep = query.getColumn(0).getInt();
        std::string data = query.getColumn(1).getString();
        inBuffer in(data);
        REF_getter<MsgEvt::BlockDBStore> bds=new MsgEvt::BlockDBStore();
        bds->unpack2(in);
        ret->blocks_Z.push_back({ep,bds});
    }
    ret->lastEpoch=root->getEpoch(NULL)->epoch;
    msg::node_message_ed nm(ret->getBuffer(),this_node_name,my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(route)));

    return true;
}


bool Node::Service::BlockAcceptedREQ(const MsgEvt::BlockAcceptedREQ* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    XTRY;

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
        do_heart_beat();
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

    REF_getter<MsgEvt::BlockDBStore> pb=new MsgEvt::BlockDBStore();
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
    REF_getter<MsgEvt::BlockAcceptedRSP> br=new MsgEvt::BlockAcceptedRSP();
    br->new_root_hash=prev_block_hash;
    br->node_signer=this_node_name;
    br->sign(my_sk_bls);
    prepared_block.clear();
    blocks.clear();

            resetTimer();

    msg::node_message_ed nm(br->getBuffer(),this_node_name,my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(route)));
    iUtils->getNow();
    XPASS;
    return true;

}
bool Node::Service::GetTransactionREQ(const MsgEvt::GetTransactionREQ* r, const NODE_id & src_node, const route_t& route)
{
            MUTEX_INSPECTOR;
            resetTimer();
            if(!root->verify_lider_certificate(r->payload_lc))
            {
                logErr2("if(!verify_lider_certificate(rft.payload_lc,node_leader))");
                return true;
            }
            
            if(root->getEpoch(NULL)->epoch < r->payload_lc->heart_beat->epoch)
            {
                    if(state_Z!=State::SYNCING)
                    {
                        // logNode("do_sync()");
                        state_Z=State::SYNCING;
                        last_leader_cert=r->payload_lc;
                        logNode("call1 do_sync();");
                        do_sync();
                        return true;
                    }
            }
            // if(src_node!=r->payload_lc->heart_beat->node_leader)
            // {
            //     logNode("messag src node != node leader %s %s",src_node.container.c_str(),r->payload_lc->heart_beat->node_leader.container.c_str());
            //     return true;
            // }
            if(blocks[prev_block_hash].heart_beat_store.node_leader!=r->payload_lc->heart_beat->node_leader)
            {
                if(isNodeGreaterThanCurrentLeader(r->payload_lc->heart_beat->node_leader))
                {
                    blocks[prev_block_hash].heart_beat_store.node_leader=r->payload_lc->heart_beat->node_leader;
                    blocks[prev_block_hash].heart_beat_store.leader_info[r->payload_lc->heart_beat->node_leader].leader_cert=r->payload_lc;
                }
                else
                {
                    logNode("invalid node cert");
                    return true;
                }
                // logNode("if(blocks[prev_block_hash].heart_beat_store.node_leader!=r->payload_lc->heart_beat->node_leader)");
            }
            sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

            // last_access_time_hbZ=time(NULL);


            if(r->payload_lc->heart_beat->prev_block_hash!=prev_block_hash) /// todo непонятно как нода узнает достоверно, что предложенный hb.prev_block_hash валиден
            {
                auto epoch=root->getEpoch(NULL);
                logNode("root->getValues(NULL)->epoch<hb.epoch %s %s",root->getEpoch(NULL)->epoch.toString().c_str(),r->payload_lc->heart_beat->epoch.toString().c_str());
                if(epoch->epoch < r->payload_lc->heart_beat->epoch)
                {
                    logNode("if(root->getValues(NULL)->epoch<hb.epoch)");
                    if(state_Z!=State::SYNCING)
                    {
                        // logNode("do_sync()");
                        state_Z=State::SYNCING;
                        last_leader_cert=r->payload_lc;
                        logNode("call2 do_sync();");
                        do_sync();
                    }
                }
                else
                {
                    logNode("invalid epoch, skipping");
                    return true;
                }

            }
            REF_getter< MsgEvt::GetTransactionRSP> rsp=new MsgEvt::GetTransactionRSP;
            for(auto &z: transaction_pool_of_leader)
            {
                rsp->trs.push_back(z.second);
            }
            msg::node_message_ed nn(rsp->getBuffer(),this_node_name,my_sk_ed);
            passEvent(new bcEvent::MsgReply(nn.getBuffer(),poppedFrontRoute(route)));

            // sendEvent(ServiceEnum::Node,new bcEvent::GetTransactions(route));
            return true;
}

bool Node::Service::ValidateBlockREQ(const MsgEvt::ValidateBlockREQ* r, const NODE_id & src_node, const route_t& route)
{

            resetTimer();
            MUTEX_INSPECTORS("ValidateBlockREQ");

            if(state_Z!=State::NORMAL)
                return true;

            if(!root->verify_lider_certificate(r->leader_cert))
                throw CommonError("if(!verify_lider_certificate(b.leader_cert))");


            if(r->leader_cert->heart_beat->prev_block_hash!=prev_block_hash)
            {
                if(root->getEpoch(NULL)->epoch<r->leader_cert->heart_beat->epoch)
                {
                    setBlockId(r->leader_cert->heart_beat->prev_block_hash);
                    return true;
                }
                logNode("ERROR: ValidateBlock block %s, nextblock %s",r->leader_cert->heart_beat->prev_block_hash.str().c_str(), prev_block_hash.str().c_str());

            }
            {

                auto new_root_hash=execute_block(root,prev_block_hash, r->transaction_bodies,r->leader_cert->nodes);
                REF_getter<MsgEvt::BlockInfo> block=new MsgEvt::BlockInfo();
                block->prev_root_hash=prev_block_hash;
                block->new_root_hash1=new_root_hash;


                block->attachment_hash.container=prepared_block.att_data.hash();

                block->payload_heart_beat=r->leader_cert->heart_beat;

                REF_getter <MsgEvt::ValidateBlockRSP> rsp=new MsgEvt::ValidateBlockRSP();
                // msg::block_response br;
                rsp->node_validator=this_node_name;
                rsp->payload_block=block;
                rsp->sign(my_sk_bls);

                msg::node_message_ed nn(rsp->getBuffer(),this_node_name,my_sk_ed);
                passEvent(new bcEvent::MsgReply(nn.getBuffer(),poppedFrontRoute(route)));


            }
            return true;

}
bool Node::Service::HeartBeatREQ(const MsgEvt::HeartBeatREQ* h,const std::string &heart_beat_payload, const route_t& route)
{
    MUTEX_INSPECTOR;
    // logNode("@@ %s",__FUNCTION__);

    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

    bool need_replace=false;
    auto &hbs=blocks[prev_block_hash].heart_beat_store;

    if(prev_block_hash!=h->prev_block_hash)
    {
        logNode("invalid root hashe, no answer");
        return true;
    }
    bool need_reply=false;
    if(h->node_leader!=hbs.node_leader)
    {
        if(isNodeGreaterThanCurrentLeader(h->node_leader))
        {
            hbs.node_leader=h->node_leader;
            need_reply=true;
        }
    }
    else
        need_reply=true;
    // if(hbs.node_leader==h->node_leader)
    // {
    //     need_reply=true;
    // }
    // else
    // {
    //     auto old_leader=root->getNode(hbs.node_leader,NULL);
    //     auto new_leader=root->getNode(h->node_leader,NULL);
    //     if(old_leader.valid() &&  new_leader.valid())
    //     {
    //         if(isNodeGreaterThanCurrentLeader(h->node_leader))
    //         {
    //             hbs.node_leader=h->node_leader;
    //             need_reply=true;
    //         }
    //     }
    // }
    auto &li=hbs.leader_info[hbs.node_leader];
    if(need_reply)
    {
        REF_getter<MsgEvt::HeartBeatRSP> hbr=new MsgEvt::HeartBeatRSP();
        // msg::heart_beat_rsp hba;
        hbr->payload_heart_beat=h;
        hbr->node_signer=this_node_name;
        hbr->signature.sign(my_sk_bls, blake2b_hash(heart_beat_payload).container);

        msg::node_message_ed nme(hbr->getBuffer(),this_node_name,my_sk_ed);
        // logNode("passEvent MsgReply %s",poppedFrontRoute(route).dump().c_str());
        passEvent(new bcEvent::MsgReply(nme.getBuffer(),poppedFrontRoute(route)));

    }
    return true;
}

bool Node::Service::Msg(const bcEvent::Msg*e, bool fromNetwork)
{

    XTRY;
    MUTEX_INSPECTOR;

    inBuffer in(e->msg);

    auto p=in.get_PN();
    switch(p)
    {
    case msgid::node_message_ed:
    {
        MUTEX_INSPECTOR;
        resetTimer();
        sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

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
        REF_getter<MsgEvt::Base> msg=msgFactory.create(p2);
        msg->unpack(in2);

        switch(msg->type)
        {
            case msgid::GetTransactionREQ:
                return GetTransactionREQ(static_cast<const MsgEvt::GetTransactionREQ*>(msg.get()),node_message_ed.src_node, e->route);
            case msgid::HeartBeatREQ:
                return HeartBeatREQ(static_cast<const MsgEvt::HeartBeatREQ*>(msg.get()),node_message_ed.payload, e->route);
            case msgid::ValidateBlockREQ:
                return ValidateBlockREQ(static_cast<const MsgEvt::ValidateBlockREQ*>(msg.get()),node_message_ed.src_node, e->route);
            case msgid::BlockAcceptedREQ:
                return BlockAcceptedREQ(static_cast<const MsgEvt::BlockAcceptedREQ*>(msg.get()),node_message_ed.src_node, e->route);
            case msgid::GetSavedBlocksREQ:
                return GetSavedBlocksREQ(static_cast<const MsgEvt::GetSavedBlocksREQ*>(msg.get()),node_message_ed.src_node, e->route);
            case msgid::DoHeartBeatREQ:
                return DoHeartBeatREQ(static_cast<const MsgEvt::DoHeartBeatREQ*>(msg.get()),node_message_ed.src_node, e->route);

                default: throw CommonError("unjandled MsgEvt %s",msgName(msg->type));
        }
    } break;
    default:
        throw CommonError("unhabdled Zp11 %s",msgName(p));
    }
    XPASS;
    return true;
}
