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
    if(li.TIMER_VALIDATE_BLOCK_DELAY_set)
    {
        logNode("TIMER_VALIDATE_BLOCK_DELAY_set is true, so do not reset timer");
        return true;
    }
    li.transaction_responders.insert(src_node);
    BigInt stake = 0;
    for (auto &z : li.transaction_responders)
    {
        auto n = root->getNode(z);
        stake += n->get_full_stake();
    }
    auto nn=root->getAllNodes();
    BigInt total_staked=0;
    for(auto &n:nn)
    {
        total_staked+=n->get_full_stake();
    }
    if (stake.toDouble() > total_staked.toDouble() * QUORUM)
    {
        auto curtime=iUtils->getNow();
        auto diff_mks=curtime-li.request_for_transactions_time;
        logNode("diff_mks %ld, stake %s, total_staked %s", diff_mks, stake.toString().c_str(), total_staked.toString().c_str());
        sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_VALIDATE_BLOCK_DELAY,NULL,NULL,double(diff_mks)/1000000., this));
        li.TIMER_VALIDATE_BLOCK_DELAY_set=true;

#ifdef KALL        
        if (hbs.leader_info.leader_cert_2.valid() && hbs.leader_info.leader_cert_2->nodes.size() == li.transaction_responders.size())
        {
            do_start_block();
            logNode("do_start_block();");
            li.transaction_responders.clear();
        }
#endif
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

    if (!r->verify(root->getNode(r->node_signer)->get_bls_pk()))
    {
        logNode("block_accepted_rsp: verify failed");
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

        agg_pk.push_back(n->get_bls_pk());
        stake += n->get_full_stake();
    }
    if (!agg_sig.verify(agg_pk, r->new_root_hash.container))
    {
        logNode("block_accepted_rsp: aggsig !veried");
        return true;
    }

    XPASS;
    return true;
}
bool Node::Service::ValidateBlockRSP(const MsgData::ValidateBlockRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if (state_Z != STATE_NORMAL)
        return true;

    if (r->blockInfo->heart_beat->prev_root_hash != prev_root_hash_Z)
    {
        logNode("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash from %s", src_node.container.c_str());
        return true;
    }
    if (!r->verify(root->getNode(r->node_validator)->get_bls_pk()))
    {
        logNode("block response not validated");
        return true;
    }

    auto &bt = blocks_leader[prev_root_hash_Z];
    auto h=r->blockInfo->getHash();
    bt.responses[h].push_back(r);
#ifdef KALL
    if (bt.responses.size())
    {
        if (bt.responses[0]->blockInfo->getHash() != r->blockInfo->getHash())
        {
            // std::cout << 
            logNode("if(bt.responses[0]->payload_block->getBuffer()!=r->getBuffer())");
            nlohmann::json j1,j2;
            bt.responses[0]->blockInfo->dump(j1);
            r->blockInfo->dump(j2);
            logNode("1::::: %s\n2:::: %s",j1.dump(2).c_str(),j2.dump(2).c_str());
            return true;
        }
    }
#endif
    if (bt.block_accepted_sent)
        return true;
    // auto & bh=bt[bl.root_hash];
    // bt.responses.push_back(r);
    BigInt stakeVal = 0;
    for (auto &z : bt.responses[h])
    {
        stakeVal += root->getNode(z->node_validator)->get_full_stake();
    }
    BigInt total_staked=0;
    auto nn=root->getAllNodes();
    for(auto& n:nn)
    {
        total_staked+=n->get_full_stake();
    }
    if (stakeVal.toDouble() > total_staked.toDouble() * QUORUM)
    {
        XTRY;
        logNode("Block stake finalized");
        REF_getter<MsgData::BlockAcceptedREQ> ba = new MsgData::BlockAcceptedREQ();
        if (!bt.blockInfo[h].valid())
        {
            bt.blockInfo[h] = r->blockInfo;
        }
        else if (bt.blockInfo[h]->getBuffer() != r->blockInfo->getBuffer())
            throw CommonError("else if(bh.block_payload!=r->payload_block)");

        ba->blockInfo = r->blockInfo;
        // if (!bt.blockInfo.valid())
        //     throw CommonError("if(!bt.block_payload.valid())");
        std::vector<blst_cpp::PublicKey> agg_pk;
        auto &hbs = blocks_leader[prev_root_hash_Z].heart_beat_store;
        // ba->leader_certificateZ = hbs.leader_info.leader_cert_2;
        for (auto &z : bt.responses[h])
        {
            auto n = root->getNode(z->node_validator);
            agg_pk.push_back(n->get_bls_pk());
            ba->agg_sig.add(z->sig);
            ba->node_validators.push_back(z->node_validator);
        }
        if (ba->agg_sig.verify(agg_pk, blake2b_hash(ba->blockInfo->getBuffer()).container))
        {
            logNode("ValidateBlockRSP block_accepted test verified OK !!!!!!!!!!!!!!!!!!!!!");
        }
        else
        {
            logNode("block_accepted verified FAIL !!!!!!!!!!!!!!!!!!!!!");
            return true;
        }
        broadcast_MsgEvent(ba.get());

        bt.block_accepted_sent = true;

        //// slash missed nodes;
        for(auto& z: bt.responses)
        {
            if(z.first!=h)
            {
                for(auto& z2: z.second)
                {
                    logNode("@@@@@@@@@@@@@@@@ slash node %s for missed block validation", z2->node_validator.container.c_str()); 
                    // auto n=root->getNode(z2->node_validator);
                    // n->inc_missed_rounds();
                    // n->setDirty();
                }
            }
            // auto n=root->getNode(z->node_validator);
            // n->set_missed_rounds(0);
            // n->setDirty();
        }
        XPASS;
    }
    XPASS;
    return true;
}

