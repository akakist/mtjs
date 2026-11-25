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
struct SubBlock
{
    // int64_t block_timestamp=0;
    // int id; /// starts with 0, -1 - last empty block
    THASH_id offer_hash;
    std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>> accepted_by_committe;
    std::map<THASH_id,std::pair<blst_cpp::AggregateSignature, std::vector<NODE_id> > > accepted_by_noncommitte;        
    bool last=false;
    THASH_id result_root_hash;
    THASH_id att_hash;

    void add_accepted_by_noncommitte(const THASH_id& blockAcceptedHash, const NODE_id& n, const blst_cpp::Signature& sig)
    {
        auto & b=accepted_by_noncommitte[blockAcceptedHash];
        for(auto & z: b.second)
        {
            if(z==n)
            {
                logErr2("already added sig");
                return;
            }
            b.first.add(sig);
            b.second.push_back(n);
        }
    }
    void add_accepted_by_committe(const REF_getter<MsgData::ValidateBlockRSP> &r)
    {
        auto h=r->blockInfo->getHash();
        auto &b=accepted_by_committe[h];
        for(auto &z:b->node_validators)
        {
            if(z==r->node_validator)
            {
                logErr2("already added validator %s",r->node_validator.container.c_str());
                return;
            }
            b->node_validators.push_back(r->node_validator);
            b->agg_sig.add(r->sig);

        }
    }
    void pack(outBuffer& o) const
    {
        o<<offer_hash<<accepted_by_committe<<accepted_by_noncommitte<<last<<result_root_hash<<att_hash;
    }
    void unpack(inBuffer& o)
    {
        o>>offer_hash>>accepted_by_committe>>accepted_by_noncommitte>>last>>result_root_hash>>att_hash;
    }
    /// @brief  do not pack next
    // bool voting_stopped=false;
    void update(Blake2bHasher& h) const
    {
        h.update(offer_hash.container);
        for(auto& z: accepted_by_committe)
        {
            h.update(offer_hash.container);
            for(auto& z: accepted_by_committe)
            {
                h.update(z.first.container);
                z.second->update(h);
            }
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
inline outBuffer& operator<<(outBuffer& o, const SubBlock& z) {
     z.pack(o); return o; 
}
inline inBuffer& operator>>(inBuffer& o, SubBlock& z) { z.unpack(o); return o; }

struct Round
{
    // int id;
    std::vector<SubBlock> subblocks;
    void update(Blake2bHasher& h) const
    {
        for(auto& z: subblocks)
        {
            z.update(h);
        }
    }

};
inline outBuffer& operator<<(outBuffer& o, const Round& z) { o<<z.subblocks; return o; }
inline inBuffer& operator>>(inBuffer& o, Round& z) { o>>z.subblocks; return o; }

/// алгоритм
/*
в хартбите делаем blocktimestamp.
Хартбит проверяется нодой если БлокСтатус уже есть. Не должен меняться.
От блоктаймстампа идут раунды.
Длительность раунда - 5 сек.
(now-bs)/duration - номер текущего раунда. 
Если раунд не завершен, то он не рассматривается.
раунд делаем параметром getMeta. То есть расклад валидаторов каждый раунд меняется.
Если валидаторы не наберут кворум, есть вероятность, что туда офлайновые попадут, то след раунд доделает.
Получается, что прежняя ротация лидера сейчас не нужна, он автоматом. вычисляется.

в бт лежит РЕФ БлокСтатуса. Если нулл, то вставляем без проверки.
Если не нулл, то проверяем одинаковость хартбита.
Subblocks кидаются последовательно по мере накопления тх.
ValidateBlockREQ - это оффер. его хеш прописываем 
в offer_hash;

Ноды на валидацию снимают диффы на каждый субблок.
Если какой-то субблок зафейлился, то его диффы потом не включаем.

Конечные

*/

namespace MsgData
{
    // Единая шина статуса: мапы агрегированных сертов по вариантам.
    // Ходит broadcast'ом (NodeMsg), ответы — отдельные события по route.
    struct BlockStatusREQ: public Base
    {
        BlockStatusREQ():Base(msgid::BlockStatusREQ) {}
        static Base* construct() { return new BlockStatusREQ(); }

        // Хеш оффера: пока его нет — ноды могут слать tx; появился — стоп.
        // bool has_offer_hash = false;
        // THASH_id offer_hash;
        // Один BlockAccepted на вариант: blockInfo + список + агрегат.
        // Ключ — hash(blockInfo); пороги (комитет 2/3, сеть 2/3) считаются по нему.
        // std::map<THASH_id, REF_getter<BlockAcceptedREQ>> accepted;

        TIME_mks block_timestamp;
        REF_getter<MsgData::HeartBeatREQ> hb;
        std::map<int,Round> rounds;

        void update(Blake2bHasher& h) const
        {
            // h.update(std::to_string((int)has_offer_hash));
            // if (has_offer_hash) h.update(offer_hash.container);
            // for (auto& z : accepted)
            // {
            //     h.update(z.first.container);
            //     if (z.second.valid()) z.second->update(h);
            // }
            h.update(block_timestamp.str());
            for(auto& z: rounds)
            {
                z.second.update(h);
            }
            hb->update(h);
            h.update(block_timestamp.str());
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<block_timestamp<<hb<<rounds;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>block_timestamp>>hb>>rounds;
        }
    };
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
