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
#include "blocks.h"
#include "route_t.h"
#include "s_ed.h"
#include "t_params.h"
#include "tools_mt.h"
#include "QUORUM.h"
#include "cells.h"
#include "proceed_merkle_on_transaction_pool_hashers.h"
#include <vector>

namespace
{
/// стейк ноды из меты; false — ноды нет в enabled-составе
bool subblock_take_stake(const REF_getter<BlockMetaFull>& mf, const NODE_id& n, i128& stake)
{
    auto it=mf->prev_stakes.find(n);
    if(it==mf->prev_stakes.end())
        return false;
    stake=it->second;
    return true;
}
}

SB_result verify_subblock_sigs(const REF_getter<MsgData::SubBlock>& b,
                               const REF_getter<MsgData::HeartBeatREQ>& hb,
                               const REF_getter<BlockMetaFull>& mf,
                               Node::Service* ns)
{
    SB_result r;

    auto log=[&](const char* what, const std::string& detail)
    {
        if(ns)
            ns->logNode("verify_subblock_sigs: %s [%s] offer=%s",
                        what, detail.c_str(), b->offer_hash.str().c_str());
    };
    auto corrupt=[&](const char* what, const std::string& detail)
    {
        log(what, detail);
        r.corrupt=true;
        return r;
    };

    if(!mf.valid())
        return corrupt("no meta", "");

    /// подписант не должен встречаться дважды ни внутри группы, ни между группами
    std::map<NODE_id,int> signed_cnt;

    /// ── подписи комитета ────────────────────────────────────────────────
    for(auto& ih: b->accepted_by_committe)
    {
        const THASH_id& h=ih.first;
        auto acc=ih.second;
        if(!acc.valid() || !acc->blockInfo.valid())
            return corrupt("committee: empty BlockAcceptedREQ", h.str());
        if(acc->blockInfo->getHash()!=h)
            return corrupt("committee: key != hash(BlockInfo)", h.str());
        if(!acc->blockInfo->offer_hash.container.empty() && acc->blockInfo->offer_hash!=b->offer_hash)
            return corrupt("committee: offer_hash mismatch", h.str());
        if(hb.valid() && acc->blockInfo->heart_beat.valid() && !acc->blockInfo->heart_beat->equals(hb))
            return corrupt("committee: heart_beat mismatch", h.str());
        if(acc->node_validators.empty())
            return corrupt("committee: no validators", h.str());

        std::vector<blst_cpp::PublicKey> pks;
        i128 stake=(i128)0;
        for(auto& n: acc->node_validators)
        {
            if(!mf->isMemberCommitte(n))
                return corrupt("committee: signer not in committee", n.container);
            if(++signed_cnt[n]>1)
                return corrupt("signer voted twice", n.container);
            auto pk=mf->getBlsPk(n);
            if(!pk)
                return corrupt("committee: no bls pk", n.container);
            pks.push_back(*pk);
            i128 s=(i128)0;
            if(!subblock_take_stake(mf,n,s))
                return corrupt("committee: no stake", n.container);
            stake+=s;
        }
        if(!acc->agg_sig.initialized())
            return corrupt("committee: empty aggregate signature", h.str());
        if(!acc->agg_sig.verify(pks,h.container))
            return corrupt("committee: aggregate signature invalid", h.str());

        if(mf->full_stake_committe>0 && (stake*100)/mf->full_stake_committe>QUORUM)
        {
            if(r.has_committe_quorum && r.committe_quorum_hash!=h)
                return corrupt("committee: quorum for two different results", h.str());
            r.has_committe_quorum=true;
            r.committe_quorum_hash=h;
        }
    }

    /// ── подписи не-комитета ─────────────────────────────────────────────
    for(auto& z: b->accepted_by_noncommitte)
    {
        const THASH_id& h=z.first;
        const auto& agg=z.second.first;
        const auto& nodes=z.second.second;
        if(nodes.empty())
            return corrupt("non-committee: no validators", h.str());

        std::vector<blst_cpp::PublicKey> pks;
        i128 stake=(i128)0;
        for(auto& n: nodes)
        {
            if(!mf->isMemberNonCommitte(n))
                return corrupt("non-committee: signer not in non-committee", n.container);
            if(++signed_cnt[n]>1)
                return corrupt("signer voted twice", n.container);
            auto pk=mf->getBlsPk(n);
            if(!pk)
                return corrupt("non-committee: no bls pk", n.container);
            pks.push_back(*pk);
            i128 s=(i128)0;
            if(!subblock_take_stake(mf,n,s))
                return corrupt("non-committee: no stake", n.container);
            stake+=s;
        }
        if(!agg.initialized())
            return corrupt("non-committee: empty aggregate signature", h.str());
        if(!agg.verify(pks,h.container))
            return corrupt("non-committee: aggregate signature invalid", h.str());

        if(mf->full_stake_all>0 && (stake*100)/mf->full_stake_all>QUORUM)
        {
            if(r.has_noncommite_quorum && r.non_committe_quorum_hash!=h)
                return corrupt("non-committee: quorum for two different results", h.str());
            r.has_noncommite_quorum=true;
            r.non_committe_quorum_hash=h;
        }
    }

    return r;
}
