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

        THASH_id offer_hash;
        std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>> accepted_by_committe;
        std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>> double_signed_by_committe;
        std::map<THASH_id,std::pair<blst_cpp::AggregateSignature, std::vector<NODE_id> > > accepted_by_noncommitte;
        std::map<THASH_id,std::pair<blst_cpp::AggregateSignature, std::vector<NODE_id> > > double_signed_by_noncommitte;
        bool last=false;
        THASH_id result_root_hash;
        THASH_id att_hash;

        void add_accepted_by_noncommitte(const THASH_id& blockAcceptedHash, const NODE_id& n, const blst_cpp::Signature& sig)
        {
            auto & b=accepted_by_noncommitte[blockAcceptedHash];
            int n_sigs=0;
            for(auto & z: b.second)
            {
                if(z==n)
                {
                    n_sigs++;
                    logErr2("already added sig");
                    return;
                }
            }
            if(n_sigs==0)
            {
                b.first.add(sig);
                b.second.push_back(n);
            }
            else{
                auto & d=double_signed_by_noncommitte[blockAcceptedHash];
                d.first.add(sig);
                d.second.push_back(n);
            }
        }
        void add_accepted_by_committe(const REF_getter<MsgData::ValidateBlockRSP> &r)
        {
            auto h=r->blockInfo->getHash();
            auto &b=accepted_by_committe[h];
            if(!b.valid())
                b=new MsgData::BlockAcceptedREQ();
            b->blockInfo=r->blockInfo;
            for(auto &z:b->node_validators)
            {
                if(z==r->node_validator)
                {
                    logErr2("already added validator %s",r->node_validator.container.c_str());
                    return;
                }
            }
            b->node_validators.push_back(r->node_validator);
            b->agg_sig.add(r->sig);
        }
        void pack(outBuffer& o) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(o);
            o<<offer_hash<<accepted_by_committe<<accepted_by_noncommitte<<last<<result_root_hash<<att_hash;
        }
        void unpack(inBuffer& o) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(o);
            o>>offer_hash>>accepted_by_committe>>accepted_by_noncommitte>>last>>result_root_hash>>att_hash;
        }
        void update(Blake2bHasher& h) const
        {
            h.update(offer_hash.container);
            for(auto& z: accepted_by_committe)
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
