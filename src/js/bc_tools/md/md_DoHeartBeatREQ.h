#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "md_LeaderCertificate.h"
namespace MsgData
{
    struct DoHeartBeatREQ: public Base
    {

        static Base* construct()
        {
            return new DoHeartBeatREQ();
        }
        DoHeartBeatREQ():Base(msgid::DoHeartBeatREQ),prev_leader_cert(new LeaderCertificate)
        {

        }
        REF_getter<LeaderCertificate> prev_leader_cert;
        void update(Blake2bHasher& h) const
        {
            throw CommonError("unimp");
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            if(prev_leader_cert.valid())
                b<<1<<prev_leader_cert;
            else b<<0;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            int valid=b.get_PN();
            if(valid)
                b>>prev_leader_cert;
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::DoHeartBeatREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::DoHeartBeatREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::DoHeartBeatREQ();
    s->unpack2(b);
    return b;
}
