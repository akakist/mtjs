#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "blst_cp.h"
namespace MsgData
{
    struct ConfirmLeaderREQ: public Base
    {

        static Base* construct()
        {
            return new ConfirmLeaderREQ();
        }
        ConfirmLeaderREQ():Base(msgid::ConfirmLeaderREQ), hb(new HeartBeatREQ)
        {

        }
        REF_getter<HeartBeatREQ> hb;
        void update(Blake2bHasher& h) const
        {
            throw CommonError("unimp");
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
    };

}

inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::ConfirmLeaderREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::ConfirmLeaderREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::ConfirmLeaderREQ();
    s->unpack2(b);
    return b;
}
