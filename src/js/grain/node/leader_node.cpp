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

bool Node::Service::GetTransactionRSP(const MsgEvt::GetTransactionRSP* r, const NODE_id & src_node, const route_t& route)
{
    XTRY;
    MUTEX_INSPECTOR;
    // logNode("GetTransactionRSP from %s", src_node.container.c_str());
            for(auto& z: r->trs)
            {
                THASH_id h=blake2b_hash(z.container);
                transaction_pool_of_leader.insert({h,z});
            }
            auto &hbs=blocks_leader[prev_block_hash].heart_beat_store;
            auto &li=hbs.leader_info;
            li.transaction_responders.insert(src_node);
            BigInt stake=0;
            for(auto &z :li.transaction_responders)
            {
                auto n=root->getNode(z,NULL);
                stake+=n->total_stake;
            }
            if(stake.toDouble() > root->getValues(NULL)->total_staked.toDouble() * QUORUM)
            {
                // for(auto &z :li.transaction_responders)
                // {
                //     logNode("transaction_responders %s",z.container.c_str());
                // }
                if(hbs.leader_info.leader_cert_2.valid() && hbs.leader_info.leader_cert_2->nodes.size()==li.transaction_responders.size())
                {
                    do_start_block();
                    logNode("do_start_block();");
                    li.transaction_responders.clear();

                }
            }
    XPASS;
            return true;
}
bool Node::Service::BlockAcceptedRSP(const MsgEvt::BlockAcceptedRSP* r, const NODE_id & src_node, const route_t& route)
{
    XTRY;
    MUTEX_INSPECTOR;
            if(!r->verify(root->getNode(r->node_signer,NULL)->bls_pk))
            {
                logErr2("block_accepted_rsp: verify failed");
                return true;
            }
            auto &bp=blocks_leader[prev_block_hash];
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
            if(root->getValues(NULL)->total_staked.toDouble()*QUORUM < stake.toDouble())
            {
                if(!bp.heart_bit_sent_on_block_accepted_rsp)
                {
                    bp.heart_bit_sent_on_block_accepted_rsp=true;
                    REF_getter<MsgEvt::DoHeartBeatREQ> rt=new MsgEvt::DoHeartBeatREQ();
                    if(!last_leader_cert.valid())
                        throw CommonError("if(!last_leader_cert.valid())");
                    rt->prev_leader_cert=last_leader_cert;
                    msg::node_message_ed nm(rt->getBuffer(),this_node_name,my_sk_ed);
                    sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(),ListenerBase::serviceId));

                    // do_heart_beat();

                }
            }
            // last_access_time_hbZ=time(NULL);


XPASS;
    return true;
}
bool Node::Service::GetSavedBlocksRSP(const MsgEvt::GetSavedBlocksRSP* r, const NODE_id & src_node, const route_t& route)
{
    XTRY;
    MUTEX_INSPECTOR;
    for(auto& z: r->blocks_Z)
    {
        // if(z.second->epoch!=z.first)
        //     throw CommonError("if(hb.epoch!=z.first)");
        if(z.second->block_accepted_req->leader_certificateZ->heart_beat->prev_block_hash!=prev_block_hash)
        {
            logNode("received invalid block %s",z.second->epoch.toString().c_str());
            continue;

        }
        else logNode("ok received block %s",z.second->epoch.toString().c_str());
            // throw CommonError("if(hb.epoch!=root->getValues(NULL)->epoch) %s %s",z.second->epoch.toString().c_str(), root->getEpoch(NULL)->epoch.toString().c_str()   );

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
        // logNode("on_get_blocks_rsp: block verified OK");


        auto new_root_hash=execute_block(root, prev_block_hash, z.second->att_data.trs,z.second->block_accepted_req->leader_certificateZ->nodes);
        if(new_root_hash==z.second->block_accepted_req->block_payload->new_root_hash1)
        {
            logNode("on_get_blocks_rsp: block executed OK on epoch %s",z.second->epoch.toString().c_str());
            auto epoch=root->getEpoch(NULL);
            // auto new_epoch=epoch->copy();
            epoch->epoch=z.second->epoch+1;
            epoch->setDirty();
            // root->setEpoch(new_epoch);
            db->write_batch(db_to_save_Z);
            db_to_save_Z.clear();

            prev_block_hash=new_root_hash;

        }
        else
        {
            throw CommonError("if(new_root_hash!=bl.new_root_hash1) %s %s",new_root_hash.str().c_str(),z.second->block_accepted_req->block_payload->new_root_hash1.str().c_str());
        }



        SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        dbs.exec("CREATE TABLE IF NOT EXISTS blocks ("
                 "epoch INTEGER PRIMARY KEY, "
                 "data BLOB NOT NULL)");

                 logNode("insert epoch %s",z.second->epoch.toString().c_str());
        SQLite::Statement insert(dbs, "REPLACE INTO blocks (epoch, data) VALUES (?, ?)");
        insert.bind(1,z.second->epoch.toString());
        insert.bind(2,z.second->getBuffer());
        insert.exec();


    }
    if(r->lastEpoch > root->getEpoch(NULL)->epoch)
    {
                                logNode("call3 do_sync();");

        logNode("do_sync again: r.lastEpoch %s > root->getValues(NULL)->epoch %s",r->lastEpoch.toString().c_str(), root->getEpoch(NULL)->epoch.toString().c_str() );
        do_sync();
        return true;
    }
    state_Z=State::NORMAL;
    logNode("State::NORMAL");
    XPASS;
    return true;
}

bool Node::Service::ValidateBlockRSP(const MsgEvt::ValidateBlockRSP* r, const NODE_id & src_node, const route_t& route)
{
    XTRY;
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
    auto & bt=blocks_leader[prev_block_hash];
    if(bt.responses.size())
    {
        if(bt.responses[0]->payload_block->getBuffer()!=r->payload_block->getBuffer())
        {
            logNode("if(bt.responses[0]->payload_block->getBuffer()!=r->getBuffer())");
            return true;
        }
    }
    if(bt.block_accepted_sent)
        return true;
    // auto & bh=bt[bl.root_hash];
    bt.responses.push_back(r);
    BigInt stakeVal=0;
    for(auto& z:bt.responses)
    {
        stakeVal+=root->getNode(z->node_validator,NULL)->total_stake;
    }
    // bt.stake_validators+=root->getNode(r->node_validator,NULL)->total_stake;
    // logNode("Block staked %lf",bt.stake.toDouble());
    if(stakeVal.toDouble() > root->getValues(NULL)->total_staked.toDouble()*QUORUM)
    {
        XTRY;
        logNode("Block stake finalized");
        REF_getter<MsgEvt::BlockAcceptedREQ>  ba=new MsgEvt::BlockAcceptedREQ();
        if(!bt.block_payload.valid())
        {
            bt.block_payload=r->payload_block;
        }
        else if(bt.block_payload->getBuffer()!=r->payload_block->getBuffer())
            throw CommonError("else if(bh.block_payload!=r->payload_block)");

        ba->block_payload=bt.block_payload;
        if(!bt.block_payload.valid())
            throw CommonError("if(!bt.block_payload.valid())");
        std::vector<blst_cpp::PublicKey> agg_pk;
        auto & hbs=blocks_leader[prev_block_hash].heart_beat_store;
        ba->leader_certificateZ=hbs.leader_info.leader_cert_2;
        for(auto& z: bt.responses)
        {
            auto n=root->getNode(z->node_validator,NULL);
            agg_pk.push_back(n->bls_pk);
            ba->agg_sig.add(z->sig);
            ba->node_validators.push_back(z->node_validator);
        }
        if(ba->agg_sig.verify(agg_pk,blake2b_hash(ba->block_payload->getBuffer()).container))
        {
            logErr2("ValidateBlockRSP block_accepted test verified OK !!!!!!!!!!!!!!!!!!!!!");
        }
        else
        {
            logErr2("block_accepted verified FAIL !!!!!!!!!!!!!!!!!!!!!");
            return true;
        }

            // outBuffer ba_buf;
            // ba->pack(ba_buf);
        // logNode("leader agg_sig %s",base62::encode(ba->agg_sig.serialize()).c_str());
        if(!ba->leader_certificateZ.valid())
            throw CommonError("if(!ba->leader_certificateZ.valid())");
        msg::node_message_ed nm(ba->getBuffer(),this_node_name,my_sk_ed);
        // make_broadcast_message(nm.getBuffer());
        sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(),ListenerBase::serviceId));

        bt.block_accepted_sent=true;
        XPASS;
    }
    XPASS;
    return true;
}


bool Node::Service::MsgReply(const bcEvent::MsgReply* e, bool fromNetwork)
{
        MUTEX_INSPECTOR;
    XTRY;
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
        sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

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

        REF_getter<MsgEvt::Base> m=msgFactory.create(p2);
        m->unpack(in2);
        switch(m->type)
        {
            case msgid::HeartBeatRSP:
                return HeartBeatRSP(static_cast<const MsgEvt::HeartBeatRSP*>(m.get()),node_message_ed.src_node, e->route);
            case msgid::ConfirmLeaderRSP:
                return ConfirmLeaderRSP(static_cast<const MsgEvt::ConfirmLeaderRSP*>(m.get()),node_message_ed.src_node, e->route);
            case msgid::GetTransactionRSP:
                return GetTransactionRSP(static_cast<const MsgEvt::GetTransactionRSP*>(m.get()),node_message_ed.src_node, e->route);
            case msgid::ValidateBlockRSP:
                return ValidateBlockRSP(static_cast<const MsgEvt::ValidateBlockRSP*>(m.get()),node_message_ed.src_node, e->route);   
            case msgid::BlockAcceptedRSP:
                return BlockAcceptedRSP(static_cast<const MsgEvt::BlockAcceptedRSP*>(m.get()),node_message_ed.src_node, e->route);   
            case msgid::GetSavedBlocksRSP:
                return GetSavedBlocksRSP(static_cast<const MsgEvt::GetSavedBlocksRSP*>(m.get()),node_message_ed.src_node, e->route);   
            default:
                throw CommonError("unhandled22 p020 %s",msgName(p2));
                break;
        }
    }
    break;
    default:
        throw CommonError("unhandled11 p %s",msgName(p));

    }
    XPASS;
    return true;
}
