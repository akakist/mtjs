#include "NODE_id.h"
#include "REF.h"
#include "blst_cp.h"
#include "blake2bHasher.h"
#include "IUtils.h"
#include "commonError.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <string>
#include <time.h>
#include "md_BlockDBStore.h"
#include "md_ValidateBlockRSP.h"
#include "ioBuffer.h"
#include "nodeService.h"
#include "route_t.h"
#include "s_ed.h"
#include "t_params.h"
#include "tools_mt.h"
#include "QUORUM.h"
#include "cells.h"
#include "proceed_merkle_on_transaction_pool_hashers.h"
#include <vector>

enum ERR_BS
{
    no_err,
    twice_signed,
    corrupt,
};
enum stage_BS
{
    allow_send_tx =  1<<0,
    allow_validate_committe = 1<<2,
};

SB_result verify_subblock(const  SubBlock &b, const REF_getter<BlockMetaFull>& mf, Node::Service* ns)
{
    SB_result r;

    std::map<NODE_id, int> sigs_cnt;
    for(auto & ih: b.accepted_by_committe)
    {
        for(auto& z: ih.second->node_validators)
        {
            sigs_cnt[z]++;
        }
    }
    for(auto &z: sigs_cnt)
    {
        if(z.second>1)
        {
            ns->logNode("twice signed");
            r.corrupt=true;
            return r;
        }
    }
    /// verify_sigs
    for(auto & ih: b.accepted_by_committe)
    {
        if(ih.second->blockInfo->getHash()!=ih.first)
        {
            ns->logNode("if(ih.second->blockInfo->getHash()!=ih.first) - broken BlockStatusREQ");
            r.corrupt=true;
            return r;
        }
        
        std::vector<blst_cpp::PublicKey> pks;
        for(auto &nname: ih.second->node_validators)
        {
            auto pk=mf->getBlsPk(nname);
            if(!pk)
            {
                r.corrupt=true;
                return r;
            }
            pks.push_back(*pk);

        }
        if(!ih.second->agg_sig.verify(pks,ih.first.container))
        {
            ns->logNode("BlockStatusREQ: verify failed");
            r.corrupt=true;
            return r;
        }

    }

    for(auto & i: b.accepted_by_committe)
    {
        i128 votes;
        for(auto& z: i.second->node_validators)
        {
            votes+=mf->getPrevStake(z);
        }
        if((votes*100)/mf->full_stake_committe > QUORUM)
        {
            r.has_committe_quorum=true;
            r.committe_quorum_hash=i.first;
        }

    }

    for(auto& z: b.accepted_by_noncommitte)
    {
        std::vector< blst_cpp::PublicKey> pks;
        for(auto&  x: z.second.second)   
        {
            auto pk=mf->getBlsPk(x);
            if(!pk)
            {
                r.corrupt=true;
            }
            pks.push_back(*pk);
        }
        if(!z.second.first.verify(pks,z.first.container))
        {
            r.corrupt=true;
        }
    }
    /// calc quorum

    for(auto& z: b.accepted_by_noncommitte)
    {
        i128 voted;
        for(auto w: z.second.second)
        {
            voted+=mf->getPrevStake(w);
        }
        if((voted*100)/mf->full_stake_all > QUORUM)
        {
            r.has_noncommite_quorum=true;
            r.non_committe_quorum_hash=z.first;
        }
    }
     return r;
}