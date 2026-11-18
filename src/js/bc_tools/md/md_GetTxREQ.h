#pragma once
#include "md_Base.h"

#include "md_HeartBeatREQ.h"
namespace MsgData
{
    struct GetTxREQ: public Base
    {

        GetTxREQ():Base(msgid::GetTxREQ),hb(new MsgData::HeartBeatREQ())
        {
        }
        REF_getter<MsgData::HeartBeatREQ> hb;
        std::string ip;

        void update(Blake2bHasher& h) const
        {
            hb->update(h);
            h.update(ip);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<hb<<ip;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>hb>>ip;
        }

        static Base* construct()
        {
            return new GetTxREQ();
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetTxREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetTxREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetTxREQ();
    s->unpack2(b);
    return b;
}
