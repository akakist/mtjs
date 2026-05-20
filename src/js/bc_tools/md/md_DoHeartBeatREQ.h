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
            prev_leader_cert->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            prev_leader_cert->unpack2(b);
        }
    };

}
