#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct ValidateBlockREQ: public Base
    {
        
        ValidateBlockREQ():Base(msgid::ValidateBlockREQ),
            leader_cert(new LeaderCertificate())
        {
        }
        static Base* construct()
        {
            return new ValidateBlockREQ();
        }

        REF_getter<LeaderCertificate>  leader_cert;
        std::vector<REF_getter<TX> > transaction_bodies;
        void update(Blake2bHasher& h) const
        {
            leader_cert->hash(h);
            for(auto& z: transaction_bodies)
            {
                z->hash(h);
            }
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            leader_cert->pack(b);
            b<<transaction_bodies.size();
            for(auto& z:transaction_bodies)
            {
                z->pack(b);
            }
            // b<<transaction_bodies;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            leader_cert->unpack2(b);
            int n=b.get_PN();
            for(int i=0;i<n;i++)
            {
                REF_getter<TX> tx=new TX;
                tx->unpack2(b);
                transaction_bodies.push_back(tx);
            }
            // b>>transaction_bodies;
        }

    };

}
