#pragma once
#include "md_Base.h"
#include "md_BlockAcceptedREQ.h"
#include "md_BlockAccepted2REQ.h"
#include <map>
#include "IUtils.h"

#define ROUND_DURATION 5000000
struct SB_result
{
    /// это вычисляемые значения
    bool vote_finished=false;
    bool corrupt=false;
    bool has_committe_quorum=false;
    THASH_id committe_quorum_hash;
    bool has_noncommite_quorum=false;
    THASH_id non_committe_quorum_hash;

};

namespace MsgData
{

    struct SubBlock: public Base
    {
        SubBlock():Base(msgid::SubBlock) {}
        static Base* construct() { return new SubBlock(); }
        int round=0;
        int subblock=0;
        THASH_id offer_hash;
        std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>> accepted_by_committe;
        std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>> double_signed_by_committe;
        /// состояние ПОСЛЕ слэша (тот же BlockInfo, но post-slash корень)
        std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>> slash;
        std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>> double_signed_slash;
        std::map<THASH_id,std::pair<blst_cpp::AggregateSignature, std::set<NODE_id> > > accepted_by_noncommitte;
        std::map<THASH_id,std::pair<blst_cpp::AggregateSignature, std::set<NODE_id> > > double_signed_by_noncommitte;
        bool last=false;
        THASH_id result_root_hash;
        THASH_id att_hash;

        void add_accepted_by_noncommitte(const THASH_id& blockAcceptedHash, const NODE_id& n, const blst_cpp::Signature& sig)
        {
            // тот же вариант — это дубль, а не эквивокация
            auto it=accepted_by_noncommitte.find(blockAcceptedHash);
            if(it!=accepted_by_noncommitte.end() && it->second.second.count(n))
                return;
            // нода уже подписала ДРУГОЙ вариант — эквивокация
            for(auto & z: accepted_by_noncommitte)
                if(z.first!=blockAcceptedHash && z.second.second.count(n))
                {
                    auto & d=double_signed_by_noncommitte[blockAcceptedHash];
                    if(d.second.count(n))
                        return;   // дубль второй подписи
                    d.first.add(sig);
                    d.second.insert(n);
                    return;
                }
            auto & b=accepted_by_noncommitte[blockAcceptedHash];
            b.first.add(sig);
            b.second.insert(n);
        }
        // Эквивокация детектится ТОЛЬКО внутри одной мапы (acc/dbl).
        // accept и slash — разные мапы, поэтому нода, подписавшая и A, и B,
        // эквивокатором не считается.
        void add_accepted_generic(std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>>& acc,
                                  std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>>& dbl,
                                  const REF_getter<MsgData::ValidateBlockRSP> &r,
                                  const THASH_id& winner)
        {
            auto h=r->blockInfo->getHash();
            // кворум уже набран: принимаем только подписи победившего хеша
            if(!winner.container.empty() && winner != h)
                return;
            // тот же BlockInfo — это дубль, а не эквивокация
            auto it=acc.find(h);
            if(it!=acc.end() && it->second.valid() && it->second->node_validators.count(r->node_validator))
                return;
            // нода уже подписала ДРУГОЙ BlockInfo в этой же мапе — эквивокация
            for(auto & z: acc)
                if(z.first!=h && z.second.valid() && z.second->node_validators.count(r->node_validator))
                {
                    auto &d=dbl[h];
                    if(!d.valid())
                    {
                        d=new MsgData::BlockAcceptedREQ();
                        d->blockInfo=r->blockInfo;
                    }
                    if(d->node_validators.count(r->node_validator))
                        return;   // дубль второй подписи
                    d->node_validators.insert(r->node_validator);
                    d->agg_sig.add(r->sig);
                    return;
                }
            auto & b=acc[h];
            if(!b.valid())
                b=new MsgData::BlockAcceptedREQ();
            b->blockInfo=r->blockInfo;
            b->node_validators.insert(r->node_validator);
            b->agg_sig.add(r->sig);
        }
        void add_accepted_by_committe(const REF_getter<MsgData::ValidateBlockRSP> &r, const THASH_id& winner)
        {
            add_accepted_generic(accepted_by_committe, double_signed_by_committe, r, winner);
        }
        void add_slash(const REF_getter<MsgData::ValidateBlockRSP> &r)
        {
            add_accepted_generic(slash, double_signed_slash, r, THASH_id());
        }
        void pack(outBuffer& o) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(o);
            o<<offer_hash<<accepted_by_committe<<double_signed_by_committe
             <<slash<<double_signed_slash
             <<accepted_by_noncommitte<<double_signed_by_noncommitte<<last<<result_root_hash<<att_hash;
        }
        void unpack(inBuffer& o) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(o);
            o>>offer_hash>>accepted_by_committe>>double_signed_by_committe
             >>slash>>double_signed_slash
             >>accepted_by_noncommitte>>double_signed_by_noncommitte>>last>>result_root_hash>>att_hash;
        }
        void update(Blake2bHasher& h) const
        {
            h.update(offer_hash.container);
            for(auto& z: accepted_by_committe)
            {
                h.update(z.first.container);
                z.second->update(h);
            }
            for(auto& z: double_signed_by_committe)
            {
                h.update(z.first.container);
                z.second->update(h);
            }
            for(auto& z: slash)
            {
                h.update(z.first.container);
                z.second->update(h);
            }
            for(auto& z: double_signed_slash)
            {
                h.update(z.first.container);
                z.second->update(h);
            }
            for(auto& z: accepted_by_noncommitte)
            {
                h.update(z.first.container);
                h.update(z.second.first.serialize());
                for(auto& xx: z.second.second)
                {
                    h.update(xx.container);
                }
            }
            for(auto& z: double_signed_by_noncommitte)
            {
                h.update(z.first.container);
                h.update(z.second.first.serialize());
                for(auto& xx: z.second.second)
                {
                    h.update(xx.container);
                }
            }
            h.update(std::to_string(last));
            h.update(result_root_hash.container);
            h.update(att_hash.container);
        }
    };

    struct Round: public Base
    {
        Round():Base(msgid::Round) {}
        static Base* construct() { return new Round(); }

        std::vector<REF_getter<SubBlock>> subblocks;
        /// true — субблоков больше не будет: лидер вышел за половину слота либо пул пуст.
        /// Клиент, увидев finished, понимает, что можно переходить к финализации.
        bool finished=false;
        NODE_id leader;
        TIME_mks round_timestamp;

        /// Достать/создать субблок по индексу (REF_getter: вектор хранит указатели).
        REF_getter<SubBlock> getOrCreateSubblock(int idx)
        {
            if(idx<0) return nullptr;
            if(subblocks.size()<=(size_t)idx)
                subblocks.resize((size_t)idx+1);
            if(!subblocks[idx].valid())
                subblocks[idx]=new SubBlock();
            return subblocks[idx];
        }

        void pack(outBuffer& o) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(o);
            o<<subblocks<<finished<<leader<<round_timestamp;
        }
        void unpack(inBuffer& o) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(o);
            o>>subblocks>>finished>>leader>>round_timestamp;
        }
        void update(Blake2bHasher& h) const
        {
            for(auto& z: subblocks)
            {
                z->update(h);
            }
            h.update(std::to_string(finished));
            h.update(leader.container);
            h.update(round_timestamp.str());
        }
    };

    // Единая шина статуса: мапы агрегированных сертов по вариантам.
    // Ходит broadcast'ом (NodeMsg), ответы — отдельные события по route.
    struct BlockStatusREQ: public Base
    {
        BlockStatusREQ():Base(msgid::BlockStatusREQ) {}
        static Base* construct() { return new BlockStatusREQ(); }

        REF_getter<MsgData::HeartBeatREQ> hb;
        std::map<int, REF_getter<Round>> rounds;

        void update(Blake2bHasher& h) const
        {
            for(auto& z: rounds)
            {
                z.second->update(h);
            }
            hb->update(h);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<hb<<rounds;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>hb>>rounds;
        }
    };
}

inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::SubBlock> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::SubBlock> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::SubBlock();
    s->unpack2(b);
    return b;
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::Round> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::Round> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::Round();
    s->unpack2(b);
    return b;
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockStatusREQ> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::BlockStatusREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::BlockStatusREQ();
    s->unpack2(b);
    return b;
}
