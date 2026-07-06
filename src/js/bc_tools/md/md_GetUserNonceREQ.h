#pragma once
#include "md_Base.h"
#include "md_LeaderCertificate.h"
namespace MsgData
{
    struct GetUserNonceREQ: public Base
    {

        GetUserNonceREQ():Base(msgid::GetUserNonceREQ)
        {
        }
        ADDRESS_id user_address;
        std::string rnd;
        void update(Blake2bHasher& h) const
        {
            h.update(user_address.addr);
            h.update(rnd);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<user_address;
            b<<rnd;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>user_address;
            b>>rnd;
        }

        static Base* construct()
        {
            return new GetUserNonceREQ();
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetUserNonceREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetUserNonceREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetUserNonceREQ();
    s->unpack2(b);
    return b;
}
