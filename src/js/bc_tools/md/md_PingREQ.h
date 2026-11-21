#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
namespace MsgData
{
    struct PingREQ: public Base
    {
        PingREQ():Base(msgid::PingREQ),hb(new MsgData::HeartBeatREQ())
        {
        }
        REF_getter<MsgData::HeartBeatREQ> hb;
        void update(Blake2bHasher& h) const
        {
            hb->update(h);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<hb;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>hb;
        }
        static Base* construct()
        {
            return new PingREQ();
        }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::PingREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::PingREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::PingREQ();
    s->unpack2(b);
    return b;
}
