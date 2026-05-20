#pragma once
#include "md_Base.h"
#include "md_LeaderCertificate.h"
namespace MsgData
{
    struct GetTransactionREQ: public Base
    {
        
        GetTransactionREQ():Base(msgid::GetTransactionREQ),lc(new LeaderCertificate())
        {
        }
        REF_getter<LeaderCertificate> lc;
        void update(Blake2bHasher& h) const
        {
            lc->hash(h);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            lc->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            lc->unpack2(b);
        }

        static Base* construct()
        {
            return new GetTransactionREQ();
        }
    };

}
