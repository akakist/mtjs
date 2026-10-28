#pragma once
#include "md_Base.h"

#include "md_HeartBeatREQ.h"
namespace MsgData
{
    struct GetTransactionREQ: public Base
    {

        GetTransactionREQ():Base(msgid::GetTransactionREQ),hb(new MsgData::HeartBeatREQ())
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
            return new GetTransactionREQ();
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetTransactionREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetTransactionREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetTransactionREQ();
    s->unpack2(b);
    return b;
}
