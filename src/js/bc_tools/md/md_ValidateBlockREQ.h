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
        std::vector<REF_getter<MsgData::TX> > transaction_bodies;
        void update(Blake2bHasher& h) const
        {
            leader_cert->update(h);
            for(auto& z: transaction_bodies)
            {
                // z->update(h);
                // leader_cert->update(h);
                h.update(z->hash.container);
            }
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<leader_cert;
            b<<transaction_bodies;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>leader_cert;
            b>>transaction_bodies;
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
