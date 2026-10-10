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
    XTRY;
    MUTEX_INSPECTOR;
    if(!db_state->sync_empty)
    {
        logNode("GetTransactionRSP if(!db_state->sync_empty)");
        return true;
    }


    for (auto &z : r->trs)
    {
        THASH_id h = z->getHash();
        transaction_pool_of_leader.insert({h, z});
    }
    auto &li = l_blocks[prev_root_hash_Z()].leader_info;
    if(iUtils->getNow()-li.TIMER_VALIDATE_BLOCK_DELAY_set < _1sec)
    {
        return true;
    }
    li.transaction_responders.insert(src_node);
    auto mf=getMetaFull(li.leader_cert_2->block_timestamp);

    uint64_t fullstake = 0;

    for(auto& z: mf->all_nodes_enabled)
    {
        fullstake+=mf->getPrevStake(z);
    }
    uint64_t stake = 0;
    for (auto &z : li.transaction_responders)
    {
        
        auto n = db_state->getNodeNoCreate(z);
        if(!n.valid())
            throw CommonError("if(!n.valid())");

        stake += n->get_full_stake();
    }
    auto pers=(stake*100)/fullstake;

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
    if(!db_state->sync_empty)
    {
        logNode("ValidateBlockRSP if(!db_state->sync_empty)");
        return true;
    }
    auto& bt=l_blocks[prev_root_hash_Z()];

    if (bt.blockDiffValidateREQ->blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1 != prev_root_hash_Z())
    {
        logNode("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash from %s", src_node.container.c_str());
        return true;
    }
    auto nnn=db_state->getNodeNoCreateConst(r->node_validator);
    if(!nnn.valid())
        throw CommonError("if(!nnn.valid())");
    if (!r->sig.verify(nnn->get_bls_pk(),bt.blockDiffValidateREQ->blockAcceptedREQ->getHash().container))
    {
        logNode("block response not validated");
        return true;
    }
    auto h=bt.blockDiffValidateREQ->blockAcceptedREQ->getHash();
    bt.BlockDiffValidateRSP_m[h].push_back(r);


    uint64_t fullstakeVal = 0;
    auto ls=db_state->getAllNodes();
    for (auto &z : ls)
    {
        if(z->isEnabled())
            fullstakeVal += z->get_full_stake();
    }
    

    uint64_t stakeVal = 0;
    for (auto &z : bt.BlockDiffValidateRSP_m[h])
    {
        stakeVal += db_state->getNodeNoCreateConst(z->node_validator)->get_full_stake();
    }
    std::set<NODE_id> hb_live;
    for(auto& z: bt.leader_info.HeartBeatRSP_m)
    {
        hb_live.insert(z.first);
    }
    std::set<NODE_id> diffreplied;

    for(auto& z: bt.BlockDiffValidateRSP_m[h])
    {
        diffreplied.insert(z->node_validator);
    }
    if ((stakeVal * 100) / fullstakeVal > QUORUM && iUtils->getNow()-bt.blockAccepted2REQ_sent > BLOCK_ACCEPTED_SENT_TIMEOUT * _1sec)
    {
    MUTEX_INSPECTOR;
        XTRY;
        bt.blockAccepted2REQ_sent=iUtils->getNow();
        REF_getter<MsgData::BlockAccepted2REQ> ba2 = new MsgData::BlockAccepted2REQ();

        ba2->blockAcceptedREQ = bt.blockDiffValidateREQ->blockAcceptedREQ;
        std::vector<blst_cpp::PublicKey> agg_pk;
        std::set<std::string> nnn;
        for (auto &z : bt.BlockDiffValidateRSP_m[h])
        {
            auto n = db_state->getNodeNoCreateConst(z->node_validator);
            agg_pk.push_back(n->get_bls_pk());
            ba2->agg_diff_sig.add(z->sig);
            ba2->node_diff_validators.push_back(z->node_validator);
            nnn.insert(z->node_validator.container);
        }
        if (ba2->agg_diff_sig.verify(agg_pk, bt.blockDiffValidateREQ->blockAcceptedREQ->getHash().container))
        {
        }
        else
        {
            throw CommonError("BlockDiffValidateRSP verified FAIL !!!!!!!!!!!!!!!!!!!!!");
            return true;
        }

        if(src_node==bt.blockDiffValidateREQ->blockAcceptedREQ->blockInfo->heart_beat->node_leader)
            logNode("validators %s",iUtils->join(" ",nnn).c_str());


        std::string nodelist;
        for(auto &z: bt.leader_info.HeartBeatRSP_m)
        {
            nodelist+=z.first.container+" ";
        }
        if(src_node==bt.blockDiffValidateREQ->blockAcceptedREQ->blockInfo->heart_beat->node_leader)
            logNode("hb list %s",nodelist.c_str());

        REF_getter<MsgData::BlockDBStore> bds=new MsgData::BlockDBStore;
        // bds->att_data=r->
        bds->blockAccepted2REQ=ba2;
        bds->tx_hashes=bt.blockDiffValidateREQ->tx_hashes;
        bds->diffs=bt.blockDiffValidateREQ->diffs;
        bds->att_data=bt.blockDiffValidateREQ->att_data;
        logNode("broadcast MsgData::BlockDBStore");
        auto mf=getMetaFull(bt.blockDiffValidateREQ->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp);
        broadcast_MsgEvent_via_broadcaster(bds.get(),mf->tree_all_nodes);
        XPASS;
    }

    
    return true;
}

bool Node::Service::ValidateBlockRSP(const MsgData::ValidateBlockRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
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
    auto nnn=db_state->getNodeNoCreate(r->node_validator);
    if(!nnn.valid())
        throw CommonError("if(!nnn.valid())");
    if (!r->verify(nnn->get_bls_pk()))
    {
        logNode("block response not validated");
        return true;
    }

    auto &bt = l_blocks[prev_root_hash_Z()];
    auto h=r->blockInfo->getHash();

    bt.ValidateBlockRSP_m[h].push_back(r);
    if ( iUtils->getNow() < bt.blockDiffValidateREQ_sent +_1sec)
        return true;
    auto mf=getMetaFull(r->blockInfo->heart_beat->block_timestamp);
    uint64_t stakeVal = 0;
    // logErr2("val node %s", src_node.container.c_str());
    for (auto &z : bt.ValidateBlockRSP_m[h])
    {
        stakeVal += mf->getPrevStake(z->node_validator);;
    }
    uint64_t fullstake=0;
    for(auto &z: mf->committe_members)
    {
        fullstake += mf->getPrevStake(z);

    }
    
    if ((stakeVal * 100) / fullstake > QUORUM )
    {
        XTRY;
        // logNode("Block stake finalized %lld ",(stakeVal * 100) / fullstake);
        REF_getter<MsgData::BlockAcceptedREQ> ba = new MsgData::BlockAcceptedREQ();
        if (!bt.blockInfo_Z[h].valid())
        {
            bt.blockInfo_Z[h] = r->blockInfo;
        }
        else if (bt.blockInfo_Z[h]->getBuffer() != r->blockInfo->getBuffer())
            throw CommonError("else if(bh.block_payload!=r->payload_block)");

        ba->blockInfo = r->blockInfo;
        std::vector<blst_cpp::PublicKey> agg_pk;
        std::set<std::string> nnn;
        std::string nv_;
        for (auto &z : bt.ValidateBlockRSP_m[h])
        {
            nv_+=z->node_validator.container+" ";
            auto n = db_state->getNodeNoCreateConst(z->node_validator);
            auto st=mf->getPrevStake(z->node_validator);
            nv_+=std::to_string(st)+" ";
            agg_pk.push_back(n->get_bls_pk());
            ba->agg_sig.add(z->sig);
            ba->node_validators.push_back(z->node_validator);

            nnn.insert(z->node_validator.container);
        }
        if (ba->agg_sig.verify(agg_pk, ba->blockInfo->getHash().container))
        {
        }
        else
        {
            logNode("block_accepted verified FAIL !!!!!!!!!!!!!!!!!!!!!");
            return true;
        }
        REF_getter<MsgData::BlockDiffValidateREQ> bdv=new MsgData::BlockDiffValidateREQ(ba,r->tx_hashes,r->diffs,r->att_data);

        bt.blockDiffValidateREQ=bdv;


        broadcast_MsgEvent_via_broadcaster(bdv.get(),mf->tree_all_nodes);

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

