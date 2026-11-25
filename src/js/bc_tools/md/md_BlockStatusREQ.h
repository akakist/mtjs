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
    std::map<THASH_id,std::pair<blst_cpp::AggregateSignature, std::set<NODE_id> > > accepted_by_noncommitte;        
    bool last=0;
    THASH_id result_root_hash;
    THASH_id att_hash;

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
inline outBuffer& operator<<(outBuffer& o, const SubBlock& z) { o<<z.subblocks; return o; }
inline inBuffer& operator>>(inBuffer& o, SubBlock& z) { o>>z.subblocks; return o; }

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
        std::vector<Round> rounds;

        Round& current_round()
        {
            int roundNo=(iUtils->getNow()-block_timestamp).v/ROUND_DURATION;
            if(rounds.size()<roundNo+1)
            {
                rounds.resize(roundNo+1);
            }
            return rounds[roundNo];
        }
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
                z.update(h);
            }
            hb->update(h);
            h.update(block_timestamp.str());
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<(int)has_offer_hash<<offer_hash<<accepted<<hb;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            int ho=0;
            b>>ho>>offer_hash>>accepted>>hb;
            has_offer_hash=(ho!=0);
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
