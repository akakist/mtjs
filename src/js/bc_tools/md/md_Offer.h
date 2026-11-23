#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "md_TX.h"
namespace MsgData
{
    // Оффер лидера: hb + ts + тела tx. Отдельный payload (OfferREQ),
    // позже включается в attachment_data.
    struct Offer: public Base
    {
        Offer():Base(msgid::Offer) {}
        static Base* construct() { return new Offer(); }
        REF_getter<HeartBeatREQ> hb;
        TIME_mks block_timestamp;
        std::vector<REF_getter<TX>> txs;

        void update(Blake2bHasher& h) const
        {
            if (hb.valid()) hb->update(h);
            h.update(std::to_string(block_timestamp.v));
            for (auto& t : txs)
                if (t.valid()) h.update(t->getHash().container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b << hb << block_timestamp << txs;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b >> hb >> block_timestamp >> txs;
        }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::Offer> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::Offer> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::Offer();
    s->unpack2(b);
    return b;
}
