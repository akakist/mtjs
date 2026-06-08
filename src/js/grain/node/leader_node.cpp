#include "NODE_id.h"
#include "blake2bHasher.h"
#include "bigint.h"
#include "commonError.h"
#include "blst_cp.h"
#include "REF.h"
#include "corelib/mutexInspector.h"
#include <time.h>
#include "nodeService.h"
#include "QUORUM.h"
#include "route_t.h"
#include <vector>

bool Node::Service::GetTransactionRSP(const MsgData::GetTransactionRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if (state_Z != STATE_NORMAL)
        return true;

    // logNode("GetTransactionRSP from %s", src_node.container.c_str());
    for (auto &z : r->trs)
    {
        THASH_id h = z->getHash();
        transaction_pool_of_leader.insert({h, z});
    }
    auto &hbs = blocks_leader[prev_root_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    li.transaction_responders.insert(src_node);
    BigInt stake = 0;
    for (auto &z : li.transaction_responders)
    {
        auto n = root->getNode(z);
        stake += n->total_stake;
    }
    if (stake.toDouble() > root->getValues()->total_staked.toDouble() * QUORUM)
    {
        if (hbs.leader_info.leader_cert_2.valid() && hbs.leader_info.leader_cert_2->nodes.size() == li.transaction_responders.size())
        {
            do_start_block();
            logNode("do_start_block();");
            li.transaction_responders.clear();
        }
    }
    XPASS;
    return true;
}
bool Node::Service::BlockAcceptedRSP(const MsgData::BlockAcceptedRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if (state_Z != STATE_NORMAL)
        return true;

    if (!r->verify(root->getNode(r->node_signer)->bls_pk))
    {
        logErr2("block_accepted_rsp: verify failed");
        return true;
    }
    auto &bp = blocks_leader[prev_root_hash_Z];
    bp.acceptors.insert({r->node_signer, r});

    blst_cpp::AggregateSignature agg_sig;
    std::vector<blst_cpp::PublicKey> agg_pk;
    BigInt stake;
    stake = 0;
    for (auto &z : bp.acceptors)
    {
        agg_sig.add(z.second->sig_bls);
        auto n = root->getNode(z.first);
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
    if (root->getValues()->total_staked.toDouble() * QUORUM < stake.toDouble())
    {
        if (!bp.heart_bit_sent_on_block_accepted_rsp)
        {
            bp.heart_bit_sent_on_block_accepted_rsp = true;
            REF_getter<MsgData::DoHeartBeatREQ> rq = new MsgData::DoHeartBeatREQ();
            rq->prev_leader_cert = root->getEpoch()->prev_leader_cert;
            broadcast_MsgEvent(rq.get());
        }
    }

    XPASS;
    return true;
}
bool Node::Service::CheckState(const MsgData::HeartBeatREQ *r, const NODE_id &src_node) // 1 если невалидно
{
    if (r->prev_root_hash != prev_root_hash_Z)
    {
        if(r->prev_root_hash.container.size()==0 && prev_root_hash_Z.container.size()!=0)
            return 1;
        // logNode("if (r->prev_root_hash != prev_root_hash_Z) %s %s",r->prev_root_hash.str().c_str(),prev_root_hash_Z.str().c_str());
        if (state_Z == STATE_NORMAL)
        {
            // logNode("if (state_Z == NORMAL)");
            // r->
            // logNode("r->new_epoch %s root->getEpoch()->epoch   - %s  src_node %s",r->new_epoch.toString().c_str(), root->getEpoch()->epoch.toString().c_str(),src_node.container.c_str());
            if (r->new_epoch > root->getEpoch()->epoch + 1)
            {
                // logNode("r->new_epoch > root->getEpoch()->epoch + 1  - %s %s",r->new_epoch.toString().c_str(), root->getEpoch()->epoch.toString().c_str());
                state_Z = STATE_SYNCING;
                do_sync(src_node);
            }
            return 1;
        }
        return 1;
    }
    return 0;
}

bool Node::Service::ValidateBlockRSP(const MsgData::ValidateBlockRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if (state_Z != STATE_NORMAL)
        return true;

    if (r->blockInfo->prev_root_hash != prev_root_hash_Z)
    {
        logErr2("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash");
        return true;
    }
    if (!r->verify(root->getNode(r->node_validator)->bls_pk))
    {
        logErr2("block response not validated");
        return true;
    }

    auto &bt = blocks_leader[prev_root_hash_Z];
    if (bt.responses.size())
    {
        if (bt.responses[0]->blockInfo->getHash() != r->blockInfo->getHash())
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
        stakeVal += root->getNode(z->node_validator)->total_stake;
    }
    if (stakeVal.toDouble() > root->getValues()->total_staked.toDouble() * QUORUM)
    {
        XTRY;
        logNode("Block stake finalized");
        REF_getter<MsgData::BlockAcceptedREQ> ba = new MsgData::BlockAcceptedREQ();
        if (!bt.blockInfo.valid())
        {
            bt.blockInfo = r->blockInfo;
        }
        else if (bt.blockInfo->getBuffer() != r->blockInfo->getBuffer())
            throw CommonError("else if(bh.block_payload!=r->payload_block)");

        ba->blockInfo = bt.blockInfo;
        if (!bt.blockInfo.valid())
            throw CommonError("if(!bt.block_payload.valid())");
        std::vector<blst_cpp::PublicKey> agg_pk;
        auto &hbs = blocks_leader[prev_root_hash_Z].heart_beat_store;
        // ba->leader_certificateZ = hbs.leader_info.leader_cert_2;
        for (auto &z : bt.responses)
        {
            auto n = root->getNode(z->node_validator);
            agg_pk.push_back(n->bls_pk);
            ba->agg_sig.add(z->sig);
            ba->node_validators.push_back(z->node_validator);
        }
        if (ba->agg_sig.verify(agg_pk, blake2b_hash(ba->blockInfo->getBuffer()).container))
        {
            logErr2("ValidateBlockRSP block_accepted test verified OK !!!!!!!!!!!!!!!!!!!!!");
        }
        else
        {
            logErr2("block_accepted verified FAIL !!!!!!!!!!!!!!!!!!!!!");
            return true;
        }
        broadcast_MsgEvent(ba.get());

        bt.block_accepted_sent = true;
        XPASS;
    }
    XPASS;
    return true;
}

