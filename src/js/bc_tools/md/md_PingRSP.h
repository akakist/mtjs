#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "blst_cp.h"
namespace MsgData
{
    struct PingRSP: public Base
    {
        PingRSP():Base(msgid::PingRSP),hb(new MsgData::HeartBeatREQ())
        {
        }
        REF_getter<MsgData::HeartBeatREQ> hb;
        blst_cpp::Signature sig;   // BLS-подпись ноды над hb->getHash() (alive-сет)
        void update(Blake2bHasher& h) const
        {
            hb->update(h);
            h.update(sig.serialize());
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<hb;
            b<<sig;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>hb;
            b>>sig;
        }
        static Base* construct()
        {
            return new PingRSP();
        }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::PingRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::PingRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::PingRSP();
    s->unpack2(b);
    return b;
}
