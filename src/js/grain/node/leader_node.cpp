#include "Events/System/timerEvent.h"
#include "NODE_id.h"
#include "THASH_id.h"
#include "base62.h"
#include "blake2bHasher.h"
#include "bigint.h"
#include "commonError.h"
#include "blst_cp.h"
#include "REF.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <SQLiteCpp/Statement.h>
#include <time.h>
#include <map>
#include "msg.h"
#include "listenerBase.h"
#include "ioBuffer.h"
#include "nodeService.h"
#include "QUORUM.h"
#include "route_t.h"
#include <SQLiteCpp/Database.h>
#include <vector>

bool Node::Service::GetTransactionRSP(const MsgEvt::GetTransactionRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if (state_Z != NORMAL)
        return true;

    // logNode("GetTransactionRSP from %s", src_node.container.c_str());
    for (auto &z : r->trs)
    {
        THASH_id h = blake2b_hash(z.container);
        transaction_pool_of_leader.insert({h, z});
    }
    auto &hbs = blocks_leader[prev_block_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    li.transaction_responders.insert(src_node);
    BigInt stake = 0;
    for (auto &z : li.transaction_responders)
    {
        auto n = root->getNode(z, NULL);
        stake += n->total_stake;
    }
    if (stake.toDouble() > root->getValues(NULL)->total_staked.toDouble() * QUORUM)
    {
        // for(auto &z :li.transaction_responders)
        // {
        //     logNode("transaction_responders %s",z.container.c_str());
        // }
        if (hbs.leader_info.leader_cert_2.valid() && hbs.leader_info.leader_cert_2->nodes.size() == li.transaction_responders.size())
        {
            do_start_block();
            // logNode("do_start_block();");
            li.transaction_responders.clear();
        }
    }
    XPASS;
    return true;
}
bool Node::Service::BlockAcceptedRSP(const MsgEvt::BlockAcceptedRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if (state_Z != NORMAL)
        return true;

    if (!r->verify(root->getNode(r->node_signer, NULL)->bls_pk))
    {
        logErr2("block_accepted_rsp: verify failed");
        return true;
    }
    auto &bp = blocks_leader[prev_block_hash_Z];
    bp.acceptors.insert({r->node_signer, r});

    blst_cpp::AggregateSignature agg_sig;
    std::vector<blst_cpp::PublicKey> agg_pk;
    BigInt stake;
    stake = 0;
    for (auto &z : bp.acceptors)
    {
        agg_sig.add(z.second->sig_bls);
        auto n = root->getNode(z.first, NULL);
        if (!n.valid())
            throw CommonError("if(!n.valid())");

        agg_pk.push_back(n->bls_pk);
        stake += n->total_stake;
    }
    if (!agg_sig.verify(agg_pk, r->new_root_hash.container))
    {
        logNode("block_accepted_rsp: aggsig !veried");
        return true;
    }
    if (root->getValues(NULL)->total_staked.toDouble() * QUORUM < stake.toDouble())
    {
        if (!bp.heart_bit_sent_on_block_accepted_rsp)
        {
            bp.heart_bit_sent_on_block_accepted_rsp = true;
            REF_getter<MsgEvt::DoHeartBeatREQ> rt = new MsgEvt::DoHeartBeatREQ();
            if (!last_leader_cert.valid())
                throw CommonError("if(!last_leader_cert.valid())");
            rt->prev_leader_cert = last_leader_cert;
            msg::node_message_ed nm(rt->getBuffer(), this_node_name, my_sk_ed);
            sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(), ListenerBase::serviceId));

            // do_heart_beat();
        }
    }
    // last_access_time_hbZ=time(NULL);

    XPASS;
    return true;
}
bool Node::Service::CheckState(const MsgEvt::HeartBeatREQ *r, const NODE_id & src_node) // 1 если невалидно
{
    if(r->prev_block_hash!=prev_block_hash_Z)
    {
        if(state_Z==NORMAL)    
        {
            if(r->epoch > root->getEpoch(NULL)->epoch)
            {
                do_sync(src_node);
                state_Z=SYNCING;
            }
            return 1;
        }
        return 1;
    }
    return 0;
}


bool Node::Service::ValidateBlockRSP(const MsgEvt::ValidateBlockRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if (state_Z != NORMAL)
        return true;

    if (!r->verify(root->getNode(r->node_validator, NULL)->bls_pk))
    {
        logErr2("block response not validated");
        return true;
    }

    // msg::blockZ bl(r->payload_block);
    if (r->payload_block->prev_root_hash != prev_block_hash_Z)
    {
        logErr2("if(bl.prev_root_hash!=prev_block_hash)");
        return true;
    }
    // if(bl.)
    auto &bt = blocks_leader[prev_block_hash_Z];
    if (bt.responses.size())
    {
        if (bt.responses[0]->payload_block->getBuffer() != r->payload_block->getBuffer())
        {
            logNode("if(bt.responses[0]->payload_block->getBuffer()!=r->getBuffer())");
            return true;
        }
    }
    if (bt.block_accepted_sent)
        return true;
    // auto & bh=bt[bl.root_hash];
    bt.responses.push_back(r);
    BigInt stakeVal = 0;
    for (auto &z : bt.responses)
    {
        stakeVal += root->getNode(z->node_validator, NULL)->total_stake;
    }
    // bt.stake_validators+=root->getNode(r->node_validator,NULL)->total_stake;
    // logNode("Block staked %lf",bt.stake.toDouble());
    if (stakeVal.toDouble() > root->getValues(NULL)->total_staked.toDouble() * QUORUM)
    {
        XTRY;
        logNode("Block stake finalized");
        REF_getter<MsgEvt::BlockAcceptedREQ> ba = new MsgEvt::BlockAcceptedREQ();
        if (!bt.block_payload.valid())
        {
            bt.block_payload = r->payload_block;
        }
        else if (bt.block_payload->getBuffer() != r->payload_block->getBuffer())
            throw CommonError("else if(bh.block_payload!=r->payload_block)");

        ba->block_payload = bt.block_payload;
        if (!bt.block_payload.valid())
            throw CommonError("if(!bt.block_payload.valid())");
        std::vector<blst_cpp::PublicKey> agg_pk;
        auto &hbs = blocks_leader[prev_block_hash_Z].heart_beat_store;
        ba->leader_certificateZ = hbs.leader_info.leader_cert_2;
        for (auto &z : bt.responses)
        {
            auto n = root->getNode(z->node_validator, NULL);
            agg_pk.push_back(n->bls_pk);
            ba->agg_sig.add(z->sig);
            ba->node_validators.push_back(z->node_validator);
        }
        if (ba->agg_sig.verify(agg_pk, blake2b_hash(ba->block_payload->getBuffer()).container))
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
        if (!ba->leader_certificateZ.valid())
            throw CommonError("if(!ba->leader_certificateZ.valid())");
        msg::node_message_ed nm(ba->getBuffer(), this_node_name, my_sk_ed);
        // make_broadcast_message(nm.getBuffer());
        sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(), ListenerBase::serviceId));

        bt.block_accepted_sent = true;
        XPASS;
    }
    XPASS;
    return true;
}

bool Node::Service::MsgReply(const bcEvent::MsgReply *e, bool fromNetwork)
{
    MUTEX_INSPECTOR;
    XTRY;
    if (e->route.size())
    {
        passEvent(e);
        return true;
    }
    inBuffer in(e->msg);

    auto p = in.get_PN();
    switch (p)
    {
    case msgid::node_message_ed:
    {
        MUTEX_INSPECTOR;
        sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));

        msg::node_message_ed node_message_ed;
        node_message_ed.unpack(in);
        auto n = root->getNode(node_message_ed.src_node, NULL);
        if (!n.valid())
            throw CommonError("invalid node AAA " + node_message_ed.src_node.container);
        if (!node_message_ed.verify(n->ed_pk))
        {
            throw CommonError("if(!node_message_ed.verify_ed_pk(n->ed_pk))");
        }
        inBuffer in2(node_message_ed.payload);
        auto p2 = in2.get_PN();

        REF_getter<MsgEvt::Base> m = msgFactory.create(p2);
        m->unpack(in2);
        switch (m->type)
        {
        case msgid::HeartBeatRSP:
            return HeartBeatRSP(static_cast<const MsgEvt::HeartBeatRSP *>(m.get()), node_message_ed.src_node, e->route);
        case msgid::ConfirmLeaderRSP:
            return ConfirmLeaderRSP(static_cast<const MsgEvt::ConfirmLeaderRSP *>(m.get()), node_message_ed.src_node, e->route);
        case msgid::GetTransactionRSP:
            return GetTransactionRSP(static_cast<const MsgEvt::GetTransactionRSP *>(m.get()), node_message_ed.src_node, e->route);
        case msgid::ValidateBlockRSP:
            return ValidateBlockRSP(static_cast<const MsgEvt::ValidateBlockRSP *>(m.get()), node_message_ed.src_node, e->route);
        case msgid::BlockAcceptedRSP:
            return BlockAcceptedRSP(static_cast<const MsgEvt::BlockAcceptedRSP *>(m.get()), node_message_ed.src_node, e->route);
        case msgid::GetSavedBlocksRSP:
            return GetSavedBlocksRSP(static_cast<const MsgEvt::GetSavedBlocksRSP *>(m.get()), node_message_ed.src_node, e->route);
        default:
            throw CommonError("unhandled22 p020 %s", msgName(p2));
            break;
        }
    }
    break;
    default:
        throw CommonError("unhandled11 p %s", msgName(p));
    }
    XPASS;
    return true;
}
