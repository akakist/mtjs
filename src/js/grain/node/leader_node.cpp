#include "NODE_id.h"
#include "blake2bHasher.h"
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
    auto &bt = l_blocks[prev_root_hash_Z()];
    if(!bt.leader_cert_2.valid()) {
        logNode("GetTransactionRSP: leader_cert_2 invalid, ignore");
        return true;
    }
    bt.GetTransactionRSP_m[r->hb_hash].insert_or_assign(src_node,r);
    auto mf=getMetaFull(bt.leader_cert_2->block_timestamp);


    if (bt.is_GetTransactionRSP_m_quorum(r->hb_hash,db_state.get()))
    {
        sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_GetTransactionRSP, toRef(r->hb_hash.container), NULL,0.05, this));
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
    if(!bt.blockDiffValidateREQ.valid())
    {
        logNode("if(!bt.blockDiffValidateREQ.valid())");
        return true;

    }

    if (bt.blockDiffValidateREQ->blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1 != prev_root_hash_Z())
    {
        logNode("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash from %s", src_node.container.c_str());
        return true;
    }
    auto nnn=db_state->getNodeNoCreateConst(r->node_validator);
    if(!nnn.valid())
        throw CommonError("if(!nnn.valid()) @44");
    if (!r->sig.verify(nnn->get_bls_pk(),bt.blockDiffValidateREQ->blockAcceptedREQ->getHash().container))
    {
        logNode("block response not validated");
        return true;
    }
    auto h=r->hb_hash;
    bt.BlockDiffValidateRSP_m[h].push_back(r);


    if(bt.is_BlockDiffValidateRSP_m_quorum(h,db_state.get()))
    {
        MUTEX_INSPECTOR;
        XTRY;
        REF_getter<MsgData::BlockAccepted2REQ> ba2 = new MsgData::BlockAccepted2REQ();

        ba2->blockAcceptedREQ = bt.blockDiffValidateREQ->blockAcceptedREQ;
        std::vector<blst_cpp::PublicKey> agg_pk;
        std::set<std::string> nnn;
        for (auto &z : bt.BlockDiffValidateRSP_m[h])
        {
            auto n = db_state->getNodeNoCreateConst(z->node_validator);
            if(!n.valid())
                throw CommonError("if(!n.valid()) @#1");
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
        for(auto &z: bt.HeartBeatRSP_m[h])
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
        auto h=bt.blockDiffValidateREQ->blockAcceptedREQ->blockInfo->heart_beat->getHash();

        sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(TIMER_BlockDBStore,toRef(h.container),bds.get(),0.05,this));

        XPASS;
    }


    return true;
}

bool Node::Service::verifyLeaderCert(const MsgData::LeaderCert* lc, const BlockMetaFull* mf)
{
    std::vector<blst_cpp::PublicKey> agg_pk_lc;
    uint64_t stake_lc=0;
    for(auto &z: lc->node_validators)
    {
        auto n=db_state->getNodeNoCreateConst(z);
        if(!n.valid())
            throw CommonError("if(!n.valid()) @@$8");
        agg_pk_lc.push_back(n->get_bls_pk());
        stake_lc+=mf->getPrevStake(z);
    }
    if (!lc->agg_sig.verify(agg_pk_lc, lc->hb->getHash().container))
    {
        logNode("hb aggsig not matched");
        return false;
    }
    if((stake_lc*100)/mf->fullStake()< QUORUM)
    {
        logNode("QUORUM LC failed");
        return false;
    }

    return true;
}

REF_getter<MsgData::LeaderCert> Node::Service::makeLeaderCert(const MsgData::HeartBeatREQ* hb, const BlockMetaFull* mf)
{
    auto &bt = l_blocks[prev_root_hash_Z()];
    REF_getter<MsgData::LeaderCert> lc=new MsgData::LeaderCert;
    lc->hb=hb;
    auto hb_hash=hb->getHash();
    std::vector<blst_cpp::PublicKey> agg_pk_lc;
    uint64_t stake_lc=0;
    for(auto& z:bt.ConfirmLeaderRSP_m_copy[hb_hash])
    {
        auto n=db_state->getNodeNoCreateConst(z.second->node_signer);
        if(!n.valid())
            throw CommonError("if(!n.valid()) ##124");
        agg_pk_lc.push_back(n->get_bls_pk());
        lc->agg_sig.add(z.second->signature);
        lc->node_validators.insert(z.second->node_signer);
        stake_lc+=mf->getPrevStake(z.second->node_signer);

    }
    if(!lc->agg_sig.verify(agg_pk_lc,hb_hash.container))
    {
        throw CommonError("lc verify failed");
    }
    if((stake_lc* 100)/mf->fullStake() < QUORUM)
        throw CommonError("if((stake_lc* 100)/mf->fullStake() < QUORUM) %lld",(stake_lc * 100)/mf->fullStake());

    return lc;

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
    auto h=r->blockInfo->heart_beat->getHash();

    bt.ValidateBlockRSP_m[h].push_back(r);
    auto mf=getMetaFull(r->blockInfo->heart_beat->block_timestamp);
    if(bt.is_ValidateBlockRSP_m_quorum(h,mf.get()))
    {
        XTRY;
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
            if(!n.valid())
                throw CommonError("if(!n.valid()) ^14");
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
        sendEvent(ServiceEnum::Timer,
            new timerEvent::ResetAlarm(
                TIMER_BlockDiffValidateREQ,
                NULL,
                bdv.get(),
                0.05,
                this));

        bt.blockDiffValidateREQ=bdv;



        logNode("validators %s",iUtils->join(" ",nnn).c_str());


        std::string nodelist;
        for(auto &z: bt.HeartBeatRSP_m[h])
        {
            nodelist+=z.first.container+" ";
        }
        logNode("hb list %s",nodelist.c_str());
        XPASS;
    }
    XPASS;
    return true;
}

