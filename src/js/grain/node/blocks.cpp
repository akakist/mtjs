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

SB_result Node::Service::verify_subblock_sigs(int round, int subblock, const route_t& route, const REF_getter<MsgData::SubBlock>& b,
                               const REF_getter<MsgData::HeartBeatREQ>& hb,
                               const REF_getter<BlockMetaFull>& mf)
{
    SB_result r;
    std::set<NODE_id> slashed;

    auto log=[&,b](const char* what, const std::string& detail)
    {
            logNode("verify_subblock_sigs: %s [%s] offer=%s",
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
    // std::map<NODE_id,int> signed_cnt_committe;
    // std::map<NODE_id,int> signed_cnt_noncommitte;

    /// ── подписи комитета ────────────────────────────────────────────────
    bool sign_by_me_committe_reqired=false;
    bool is_in_committe_me=mf->isMemberCommitte(this_node_name);
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

        if(!acc->node_validators.count(this_node_name) && is_in_committe_me)
            sign_by_me_committe_reqired=true;

        std::vector<blst_cpp::PublicKey> pks;
        i128 stake=(i128)0;

        for(auto& n: acc->node_validators)
        {
            if(!mf->isMemberCommitte(n))
                return corrupt("committee: signer not in committee", n.container);
            // if(++signed_cnt_committe[n]>1)
            //     return corrupt("signer voted twice", n.container);
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
    if(sign_by_me_committe_reqired)
    {
        process_offer( b->offer_hash,b, route,round, subblock);
    }
    for(auto& z:b->double_signed_by_committe)
    {
        const THASH_id& h=z.first;
        auto acc=z.second;
        if(!acc.valid() || !acc->blockInfo.valid())
            return corrupt("double_signed_by_committe: empty BlockAcceptedREQ", h.str());
        if(acc->blockInfo->getHash()!=h)
            return corrupt("double_signed_by_committe: key != hash(BlockInfo)", h.str());
        if(!acc->blockInfo->offer_hash.container.empty() && acc->blockInfo->offer_hash!=b->offer_hash)
            return corrupt("double_signed_by_committe: offer_hash mismatch", h.str());
        if(hb.valid() && acc->blockInfo->heart_beat.valid() && !acc->blockInfo->heart_beat->equals(hb))
            return corrupt("double_signed_by_committe: heart_beat mismatch", h.str());
        if(acc->node_validators.empty())
            return corrupt("double_signed_by_committe: no validators", h.str());

        std::vector<blst_cpp::PublicKey> pks;
        for(auto& n: acc->node_validators)
        {
            // эквивокация: у подписанта обязана быть и первая подпись в accepted
            bool inPrev=false;
            for(auto& a: b->accepted_by_committe)
                if(a.second.valid() && a.second->node_validators.count(n)) { inPrev=true; break; }
            if(!inPrev)
                return corrupt("double_signed_by_committe: signer not exists in accepted_by_committe", n.container);
            if(!mf->isMemberCommitte(n))
                return corrupt("double_signed_by_committe: signer not in committee", n.container);
            auto pk=mf->getBlsPk(n);
            if(!pk)
                return corrupt("double_signed_by_committe: no bls pk", n.container);
            pks.push_back(*pk);
            slashed.insert(n);   // эквивокатор — на слэш
        }
        if(!acc->agg_sig.initialized())
            return corrupt("double_signed_by_committe: empty aggregate signature", h.str());
        if(!acc->agg_sig.verify(pks,h.container))
            return corrupt("double_signed_by_committe: aggregate signature invalid", h.str());
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

            // if(++signed_cnt_noncommitte[n]>1)
            //     return corrupt("signer voted twice", n.container);
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
    for(auto& z: b->double_signed_by_noncommitte)
    {
        const THASH_id& h=z.first;
        const auto& agg=z.second.first;
        const auto& nodes=z.second.second;
        if(nodes.empty())
            return corrupt("double_signed_by_noncommitte: no validators", h.str());

        std::vector<blst_cpp::PublicKey> pks;
        for(auto& n: nodes)
        {
            // эквивокация: у подписанта обязана быть и первая подпись в accepted
            bool inPrev=false;
            for(auto& zz: b->accepted_by_noncommitte)
                if(zz.second.second.count(n)) { inPrev=true; break; }
            if(!inPrev)
                return corrupt("double_signed_by_noncommitte: signer not exists in accepted_by_noncommitte", n.container);
            if(!mf->isMemberNonCommitte(n))
                return corrupt("double_signed_by_noncommitte: signer not in non-committee", n.container);
            auto pk=mf->getBlsPk(n);
            if(!pk)
                return corrupt("double_signed_by_noncommitte: no bls pk", n.container);
            pks.push_back(*pk);
            slashed.insert(n);   // эквивокатор — на слэш
        }
        if(!agg.initialized())
            return corrupt("double_signed_by_noncommitte: empty aggregate signature", h.str());
        if(!agg.verify(pks,h.container))
            return corrupt("double_signed_by_noncommitte: aggregate signature invalid", h.str());
    }


    return r;
}
