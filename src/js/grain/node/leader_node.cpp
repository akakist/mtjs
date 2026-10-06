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
#include "md/md_BlockDiffValidateREQ.h"

bool Node::Service::GetTransactionRSP(const MsgData::GetTransactionRSP *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
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
    auto mf=getMetaFull(li.leader_cert_2->block_timestamp);
    uint64_t stake = 0;
    for (auto &z : li.transaction_responders)
    {
        
        auto n = db_state->getNodeNoCreate(z);
        if(!n.valid())
            throw CommonError("if(!n.valid())");

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
bool Node::Service::BlockDiffValidateRSP(const MsgData::BlockDiffValidateRSP* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    // logNode("@@ %s",__func__);
    if(!db_state->sync_empty)
    {
        logNode("ValidateBlockRSP if(!db_state->sync_empty)");
        return true;
    }
    auto mf=getMetaFull(r->payload_blockAcceptedREQ->blockInfo->heart_beat->block_timestamp);

    if (r->payload_blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1 != prev_root_hash_Z())
    {
        logNode("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash from %s", src_node.container.c_str());
        return true;
    }
    auto nnn=mf->getNode(r->node_validator);
    if(!nnn.valid())
        throw CommonError("if(!nnn.valid())");
    if (!r->verify(nnn->get_bls_pk()))
    {
        logNode("block response not validated");
        return true;
    }
    // logNode("verified OK %s",r->node_validator.container.c_str());
    auto &bt = l_blocks[prev_root_hash_Z()];

    // if(r->blockInfo->diff_hash!=bt.blockInfo)
    auto h=r->payload_blockAcceptedREQ->getHash();
    bt.BlockDiffValidateRSP_m[h].push_back(r);

    uint64_t stakeVal = 0;
    for (auto &z : bt.BlockDiffValidateRSP_m[h])
    {
        stakeVal += mf->getStake(z->node_validator);
    }
    // logNode("stakeVal %lld",stakeVal);
    // logNode("iUtils->getNow()-bt.blockAccepted2REQ_sent %lld",iUtils->getNow()-bt.blockAccepted2REQ_sent);
    if (stakeVal * 100 / mf->total_full_stake > QUORUM && iUtils->getNow()-bt.blockAccepted2REQ_sent > BLOCK_ACCEPTED_SENT_TIMEOUT * _1sec)
    {
    MUTEX_INSPECTOR;
        XTRY;
        bt.blockAccepted2REQ_sent=iUtils->getNow();
        logNode("Block stake finalized BlockDiffValidateRSP");
        REF_getter<MsgData::BlockAccepted2REQ> ba2 = new MsgData::BlockAccepted2REQ();

        ba2->blockAcceptedREQ = r->payload_blockAcceptedREQ;
        std::vector<blst_cpp::PublicKey> agg_pk;
        std::set<std::string> nnn;
        for (auto &z : bt.BlockDiffValidateRSP_m[h])
        {
            auto n = mf->getNode(z->node_validator);
            agg_pk.push_back(n->get_bls_pk());
            ba2->agg_diff_sig.add(z->sig);
            ba2->node_diff_validators.push_back(z->node_validator);
            nnn.insert(z->node_validator.container);
        }
        if (ba2->agg_diff_sig.verify(agg_pk, blake2b_hash(r->payload_blockAcceptedREQ->getBuffer()).container))
        {
            logNode("BlockDiffValidateRSP block_accepted test verified OK !!!!!!!!!!!!!!!!!!!!!");
        }
        else
        {
            throw CommonError("BlockDiffValidateRSP verified FAIL !!!!!!!!!!!!!!!!!!!!!");
            return true;
        }

        if(src_node==r->payload_blockAcceptedREQ->blockInfo->heart_beat->node_leader)
            logNode("validators %s",iUtils->join(" ",nnn).c_str());

        // bt.block_accepted_sent = iUtils->getNow();

        std::string nodelist;
        for(auto &z: bt.leader_info.HeartBeatRSP_m)
        {
            nodelist+=z.first.container+" ";
        }
        if(src_node==r->payload_blockAcceptedREQ->blockInfo->heart_beat->node_leader)
            logNode("hb list %s",nodelist.c_str());
        broadcast_MsgEvent_via_broadcaster(ba2.get(),mf);
        XPASS;
    }


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

    if (r->payload_blockInfo->heart_beat->prev_root_hash_1 != prev_root_hash_Z())
    {
        logNode("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash from %s", src_node.container.c_str());
        return true;
    }
    auto nnn=db_state->getNodeNoCreate(r->node_validator);
    if(!nnn.valid())
        throw CommonError("if(!nnn.valid())");
    if (!r->verify(nnn->get_bls_pk()))
    {
        logNode("block response not validated");
        return true;
    }

    auto &bt = l_blocks[prev_root_hash_Z()];
    // if(r->blockInfo->diff_hash!=bt.blockInfo)
    auto h=r->payload_blockInfo->getHash();

    bt.ValidateBlockRSP_m[h].push_back(r);
    if ( iUtils->getNow() < bt.blockDiffValidateREQ_sent +_1sec)
        return true;
    auto mf=getMetaFull(r->payload_blockInfo->heart_beat->block_timestamp);
    uint64_t stakeVal = 0;
    for (auto &z : bt.ValidateBlockRSP_m[h])
    {
        stakeVal += mf->getStake(z->node_validator);
    }
    if (stakeVal * 100 / mf->total_full_stake > QUORUM)
    {
        XTRY;
        logNode("Block stake finalized");
        REF_getter<MsgData::BlockAcceptedREQ> ba = new MsgData::BlockAcceptedREQ();
        if (!bt.blockInfo_Z[h].valid())
        {
            bt.blockInfo_Z[h] = r->payload_blockInfo;
        }
        else if (bt.blockInfo_Z[h]->getBuffer() != r->payload_blockInfo->getBuffer())
            throw CommonError("else if(bh.block_payload!=r->payload_block)");

        ba->blockInfo = r->payload_blockInfo;
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
        logNode("Broadcasr BlockDiffValidateREQ 2");
        REF_getter<MsgData::BlockDiffValidateREQ> bdv=new MsgData::BlockDiffValidateREQ;
        // bdv->blockAcceptedREQ=v_blocks[prev_root_hash_Z()].blockDBStore;
        // bt.
        // logNode("bdv->blockAcceptedREQ.valid() %d",bdv->blockAcceptedREQ.valid());
        // bt.blockInfo_Z
        auto &bv=v_blocks[prev_root_hash_Z()];
        bv.blockDBStore->blockAcceptedREQ=ba;
        bdv->blockDBStore=bv.blockDBStore;
        // bdv->diffs=bv.blockDBStore->diffs;
        // bdv->att_data=bv.blockDBStore->att_data_Z;
        // bdv->tx_hashes=bv.blockDBStore->tx_hashes;
        // if(0){
        //     /// TODO: test remove after
        //     auto buf=bdv->getBuffer();
        //     REF_getter<MsgData::BlockDiffValidateREQ> test=new MsgData::BlockDiffValidateREQ;
        //     inBuffer in2(buf);
        //     in2 >> test;
        //     // bdv->unpack2(in2);
        //     logNode("TEST MsgData::BlockDiffValidateREQ OK");
        // }


        broadcast_MsgEvent_via_broadcaster(bdv.get(),mf);

        logNode("validators %s",iUtils->join(" ",nnn).c_str());

        bt.blockDiffValidateREQ_sent = iUtils->getNow();

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

