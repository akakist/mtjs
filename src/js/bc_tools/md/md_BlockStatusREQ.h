#pragma once
#include "md_Base.h"
#include "md_BlockAcceptedREQ.h"
#include "md_BlockAccepted2REQ.h"
#include <map>


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
    int64_t block_timestamp=0;
    int id; /// starts with 0, -1 - last empty block
    THASH_id offer_hash;
    std::map<THASH_id, REF_getter<MsgData::BlockAcceptedREQ>> accepted_by_committe;
    std::map<THASH_id,std::pair<blst_cpp::AggregateSignature, std::set<NODE_id> > > accepted_by_noncommitte;        

    /// @brief  do not pack next
    // bool voting_stopped=false;

};

struct Round
{
    int id;
    std::vector<SubBlock> subblocks;
    bool finished=0;
};


namespace MsgData
{
    // Единая шина статуса: мапы агрегированных сертов по вариантам.
    // Ходит broadcast'ом (NodeMsg), ответы — отдельные события по route.
    struct BlockStatusREQ: public Base
    {
        BlockStatusREQ():Base(msgid::BlockStatusREQ) {}
        static Base* construct() { return new BlockStatusREQ(); }
        // Хеш оффера: пока его нет — ноды могут слать tx; появился — стоп.
        bool has_offer_hash = false;
        THASH_id offer_hash;
        // Один BlockAccepted на вариант: blockInfo + список + агрегат.
        // Ключ — hash(blockInfo); пороги (комитет 2/3, сеть 2/3) считаются по нему.
        std::map<THASH_id, REF_getter<BlockAcceptedREQ>> accepted;

        REF_getter<MsgData::HeartBeatREQ> hb;
        void update(Blake2bHasher& h) const
        {
            h.update(std::to_string((int)has_offer_hash));
            if (has_offer_hash) h.update(offer_hash.container);
            for (auto& z : accepted)
            {
                h.update(z.first.container);
                if (z.second.valid()) z.second->update(h);
            }
            hb->update(h);
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
