#pragma once
#include "md_Base.h"
#include "md_LeaderCertificate.h"
namespace MsgData
{
    struct DelayNotificationREQ: public Base
    {

        DelayNotificationREQ():Base(msgid::DelayNotificationREQ)
        {

        }
        static Base* construct()
        {
            return new DelayNotificationREQ();
        }
        // BigInt epoch;
        // BLOCK_id prev_root_hash;
        REF_getter<LeaderCertificate> lc;
        void update(Blake2bHasher& h) const
        {
            // h.update(epoch.toString());
            lc->update(h);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            // b<<epoch;
            b<<lc;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            // b>>epoch;
            b>>lc;
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::DelayNotificationREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::DelayNotificationREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::DelayNotificationREQ();
    s->unpack2(b);
    return b;
}
