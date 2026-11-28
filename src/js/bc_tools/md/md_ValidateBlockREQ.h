#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "md_LeaderCert.h"
namespace MsgData
{
    struct ValidateBlockREQ: public Base
    {

        ValidateBlockREQ():Base(msgid::ValidateBlockREQ)
        {
        }
        static Base* construct()
        {
            return new ValidateBlockREQ();
        }

        REF_getter<MsgData::HeartBeatREQ> heart_beat;
        std::vector<REF_getter<MsgData::TX> > transaction_bodies;
        // TIME_mks block_timestamp;
        void update(Blake2bHasher& h) const
        {
            if(heart_beat.valid())
                heart_beat->update(h);
            for(auto& z: transaction_bodies)
            {
                h.update(z->tx_body);
            }
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b << (int)(heart_beat.valid()?1:0);
            if(heart_beat.valid())
                b << heart_beat;
            b << transaction_bodies;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            int has_hb=0;
            b >> has_hb;
            if(has_hb)
                b >> heart_beat;
            b >> transaction_bodies;
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::ValidateBlockREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::ValidateBlockREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::ValidateBlockREQ();
    s->unpack2(b);
    return b;
}
