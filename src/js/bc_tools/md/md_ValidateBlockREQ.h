#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "md_LeaderCert.h"
namespace MsgData
{
    struct ValidateBlockREQ: public Base
    {

        ValidateBlockREQ():Base(msgid::ValidateBlockREQ),
            heart_beat(new MsgData::HeartBeatREQ()),
            lc(new MsgData::LeaderCert)
        {
        }
        static Base* construct()
        {
            return new ValidateBlockREQ();
        }

        REF_getter<MsgData::HeartBeatREQ>  heart_beat;
        std::vector<REF_getter<MsgData::TX> > transaction_bodies;
        REF_getter<MsgData::LeaderCert>  lc;
        void update(Blake2bHasher& h) const
        {
            heart_beat->update(h);
            for(auto& z: transaction_bodies)
            {
                
                h.update(z->tx_body);
                h.update(z->pk_ed_bin);
            }
            lc->update(h);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b << heart_beat;
            b << transaction_bodies;
            b << lc;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b >> heart_beat;
            b >> transaction_bodies;
            b >> lc;
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
