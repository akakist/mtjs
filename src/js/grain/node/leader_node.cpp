#include "NODE_id.h"
#include "blake2bHasher.h"
// #include "bigint.h"
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
    // logNode("GetTransactionRSP %s",src_node.container.c_str());

    XTRY;
    MUTEX_INSPECTOR;
    if(!db_state->sync_empty)
    {
        logNode("GetTransactionRSP if(!db_state->sync_empty)");
        return true;
    }


    // logNode("GetTransactionRSP from %s", src_node.container.c_str());
    for (auto &z : r->trs)
    {
        THASH_id h = z->getHash();
        transaction_pool_of_leader.insert({h, z});
    }
    auto &li = l_blocks[prev_root_hash_Z()].leader_info;
    // auto &li = hbs.leader_info;
    if(iUtils->getNow()-li.TIMER_VALIDATE_BLOCK_DELAY_set < _1sec)
    {
        // logNode("TIMER_VALIDATE_BLOCK_DELAY_set is true, so do not reset timer");
        return true;
    }
    li.transaction_responders.insert(src_node);
    auto mf=getMetaFull();
    uint64_t stake = 0;
    for (auto &z : li.transaction_responders)
    {
        
        auto n = root->getNode(z,db_state.get());
        stake += mf->getStake(z);
    }
    auto pers=(stake*100)/mf->total_full_stake;

    if (pers >  QUORUM)
    {
        auto curtime=iUtils->getNow();
        auto diff_mks=curtime-li.request_for_transactions_time;
        sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_VALIDATE_BLOCK_DELAY,NULL,NULL,double(diff_mks)/1000000., this));
        li.TIMER_VALIDATE_BLOCK_DELAY_set=iUtils->getNow();

    }
    XPASS;
    return true;
}

bool Node::Service::ValidateBlockRSP(const MsgData::ValidateBlockRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    // logNode("@@ %s",__func__);
    if(!db_state->sync_empty)
    {
        logNode("ValidateBlockRSP if(!db_state->sync_empty)");
        return true;
    }

    if (r->blockInfo->heart_beat->prev_root_hash_1 != prev_root_hash_Z())
    {
        logNode("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash from %s", src_node.container.c_str());
        return true;
    }
    if (!r->verify(root->getNode(r->node_validator,db_state.get())->get_bls_pk()))
    {
        logNode("block response not validated");
        return true;
    }

    auto &bt = l_blocks[prev_root_hash_Z()];
    auto h=r->blockInfo->getHash();
    bt.ValidateBlockRSP_m[h].push_back(r);
    if ( iUtils->getNow() < bt.block_accepted_sent +_1sec)
        return true;
    auto mf=getMetaFull();
    #ifndef FULL_M
    auto mv=getMetaValidator(bt.leader_info.leader_cert_2->block_timestamp);
    #endif
    uint64_t stakeVal = 0;
    for (auto &z : bt.ValidateBlockRSP_m[h])
    {
        stakeVal += mf->getStake(z->node_validator);
    }
#ifdef FULL_M
    if (stakeVal * 100 / mf->total_full_stake > QUORUM)
#else
    if (stakeVal * 100 / mv->total_validator_stake > QUORUM)
#endif
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
        std::vector<blst_cpp::PublicKey> agg_pk;
        std::set<std::string> nnn;
        for (auto &z : bt.ValidateBlockRSP_m[h])
        {
            auto n = mf->getNode(z->node_validator);
            agg_pk.push_back(n->get_bls_pk());
            ba->agg_sig.add(z->sig);
            ba->node_validators.push_back(z->node_validator);
            nnn.insert(z->node_validator.container);
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
        
        broadcast_MsgEvent(ba.get(),getMetaFull()->full_broadcast);

        logNode("validators %s",iUtils->join(" ",nnn).c_str());

        bt.block_accepted_sent = iUtils->getNow();

        std::string nodelist;
        for(auto &z: bt.leader_info.HeartBeatRSP_m)
        {
            nodelist+=z.first.container+" ";
        }
        logNode("hb list %s",nodelist.c_str());
        XPASS;
    }
    XPASS;
    return true;
}

