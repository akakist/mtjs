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
    // if(!bt.leader_cert_2.valid()) {
    //     logNode("GetTransactionRSP: leader_cert_2 invalid, ignore");
    //     return true;
    // }
    auto h=r->hb->getHash();
    bt.GetTransactionRSP_m[h].insert_or_assign(src_node,r);
    auto mf=getMetaFull(r->hb->block_timestamp);


    if (bt.is_GetTransactionRSP_m_quorum(h,root.get(), db_state.get()))
    {
        sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_GetTransactionRSP, toRef(h.container), r->hb.get(),0.05, this));
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
    auto& ba_hash=r->blockAcceptedHash;
    auto br=bt.blockDiffValidateREQ_2[ba_hash];
    if(!br.valid())
    {
        logNode("if(!bt.blockDiffValidateREQ.valid())");
        return true;

    }


    if (br->blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1 != prev_root_hash_Z())
    {
        logNode("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash from %s", src_node.container.c_str());
        return true;
    }
    auto nnn=cells::getNodeNoCreateConst(root.get(),db_state.get(),r->node_validator);
    if(!nnn.valid())
        throw CommonError("if(!nnn.valid()) @44");


    // DIAG: what leader verifies
    {
        DBG(logNode("[DIAG BlockDiffValidateRSP verify] validator=%s pk=%s sig=%s hash=%s",
            r->node_validator.container.c_str(),
            base16::encode(nnn->get_bls_pk().serialize()).c_str(),
            base16::encode(r->sig34.serialize()).c_str(),
            br->blockAcceptedREQ->getHash().str().c_str()));
    }
    if(br->blockAcceptedREQ->getHash()!=r->blockAcceptedHash)
    {
        logNode("if(br->blockAcceptedREQ->getHash()!=r->blockAcceptedHash)");
        return true;
    }

    if (!r->sig34.verify(nnn->get_bls_pk(),r->blockAcceptedHash.container))
    {
        logNode("[BlockDiffValidateRSP] block response not validated (validator=%s, pk=%s, hash=%s)",
            r->node_validator.container.c_str(),
            base16::encode(nnn->get_bls_pk().serialize()).c_str(),
            br->blockAcceptedREQ->getHash().str().c_str());
        return true;
    }
    // auto h=r->blockInfoHash;
    // logNode("[BlockDiffValidateRSP] blockInfoHash %s from %s",r->blockInfoHash.str().c_str(),src_node.container.c_str());
    {
        DBG(logNode("KALL BDRSP push: r->blockAcceptedHash=%s br->getHash=%s nnn_validator=%s",
                r->blockAcceptedHash.str().c_str(),
                br->blockAcceptedREQ->getHash().str().c_str(),
                r->node_validator.container.c_str()));
    }
    bt.BlockDiffValidateRSP_m[r->blockAcceptedHash].push_back(r);
    DBG(logNode("KALL BDRSP push done: key=%s size=%d",
        r->blockAcceptedHash.str().c_str(),
        (int)bt.BlockDiffValidateRSP_m[r->blockAcceptedHash].size()));


    if(bt.is_BlockDiffValidateRSP_m_quorum(r->blockAcceptedHash,root.get(),db_state.get()))
    {
        MUTEX_INSPECTOR;
        XTRY;
        REF_getter<MsgData::BlockAccepted2REQ> ba2 = new MsgData::BlockAccepted2REQ();

        ba2->blockAcceptedREQ = br->blockAcceptedREQ;
        std::vector<blst_cpp::PublicKey> agg_pk;
        std::set<std::string> nnn;
        for (auto &z : bt.BlockDiffValidateRSP_m[r->blockAcceptedHash])
        {
            auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(),z->node_validator);
            if(!n.valid())
                throw CommonError("if(!n.valid()) @#1");
            agg_pk.push_back(n->get_bls_pk());
            ba2->agg_diff_sig.add(z->sig34);
            ba2->node_diff_validators.push_back(z->node_validator);
            nnn.insert(z->node_validator.container);
        }
        if (ba2->agg_diff_sig.verify(agg_pk, r->blockAcceptedHash.container))
        {
        }
        else
        {
            throw CommonError("BlockDiffValidateRSP verified FAIL !!!!!!!!!!!!!!!!!!!!!");
            return true;
        }

        if(src_node==br->blockAcceptedREQ->blockInfo->heart_beat->node_leader)
            logNode("validators %s",iUtils->join(" ",nnn).c_str());


        std::string nodelist;
        if(bt.ConfirmLeaderRSP_m_copy.size())
        {
            auto cfh=bt.ConfirmLeaderRSP_m_copy.begin()->first;
            for(auto &z: bt.ConfirmLeaderRSP_m_copy[cfh])
            {
                nodelist+=z.first.container+" ";
            }
        }
        if(src_node==br->blockAcceptedREQ->blockInfo->heart_beat->node_leader)
            logNode("hb list %s",nodelist.c_str());

        REF_getter<MsgData::BlockDBStore> bds=new MsgData::BlockDBStore;
        // bds->att_data=r->
        bds->blockAccepted2REQ=ba2;
        bds->tx_hashes=br->tx_hashes;
        bds->diffs=br->diffs;
        bds->att_data=br->att_data;
        auto h=br->blockAcceptedREQ->getHash();

        sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(TIMER_BlockDBStore,toRef(h.container),bds.get(),0.05,this));

        XPASS;
    }



    return true;
}

bool Node::Service::verifyLeaderCert(const MsgData::LeaderCert* lc, const BlockMetaFull* mf)
{
    std::vector<blst_cpp::PublicKey> agg_pk_lc;
    int64_t stake_lc=0;
    for(auto &z: lc->node_validators)
    {
        auto n=cells::getNodeNoCreateConst(root.get(),db_state.get(), z);
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
    int64_t stake_lc=0;
    for(auto& z:bt.ConfirmLeaderRSP_m_copy[hb_hash])
    {
        auto n=cells::getNodeNoCreateConst(root.get(), db_state.get(),z.second->node_signer);
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
    auto nnn=cells::getNodeNoCreate(root.get(), db_state.get(),r->node_validator);
    if(!nnn.valid())
        throw CommonError("if(!nnn.valid())");
    if (!r->verify(nnn->get_bls_pk()))
    {
        logNode("[ValidateBlockRSP] block response not validated ");
        return true;
    }

    auto &bt = l_blocks[prev_root_hash_Z()];
    auto h=r->blockInfo->getHash();
    // logNode("blockInfo->getHash() %s from node %s",h.str().c_str(),src_node.container.c_str());
    bt.ValidateBlockRSP_m[h].push_back(r);
    auto mf=getMetaFull(r->blockInfo->heart_beat->block_timestamp);
    if(bt.is_ValidateBlockRSP_m_quorum(h,mf.get()))
    {
        XTRY;
        REF_getter<MsgData::BlockAcceptedREQ> ba = new MsgData::BlockAcceptedREQ();
        // if (!bt.blockInfo_Z[h].valid())
        // {
        //     bt.blockInfo_Z[h] = r->blockInfo;
        // }
        // else if (bt.blockInfo_Z[h]->getBuffer() != r->blockInfo->getBuffer())
        //     throw CommonError("else if(bh.block_payload!=r->payload_block)");

        ba->blockInfo = r->blockInfo;
        std::vector<blst_cpp::PublicKey> agg_pk;
        std::set<std::string> nnn;
        std::string nv_;
        for (auto &z : bt.ValidateBlockRSP_m[h])
        {
            nv_+=z->node_validator.container+" ";
            auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(),z->node_validator);
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
            logNode("block_accepted verified FAIL !!!!!!!!!!!!!!!!!!!!! val count %d",bt.ValidateBlockRSP_m[h].size());
            return true;
        }
        REF_getter<MsgData::BlockDiffValidateREQ> bdv=new MsgData::BlockDiffValidateREQ(ba,r->tx_hashes,r->diffs,r->deleted, r->att_data);
        sendEvent(ServiceEnum::Timer,
            new timerEvent::ResetAlarm(
                TIMER_BlockDiffValidateREQ,
                NULL,
                bdv.get(),
                0.05,
                this));

        bt.blockDiffValidateREQ_2[bdv->blockAcceptedREQ->getHash()]=bdv;



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

