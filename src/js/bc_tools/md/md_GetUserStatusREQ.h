#pragma once
#include "md_Base.h"
#include "md_LeaderCertificate.h"
namespace MsgData
{
    struct GetUserStatusREQ: public Base
    {

        GetUserStatusREQ():Base(msgid::GetUserStatusREQ)
        {
        }
        std::string user_pk_hex_ed;
        std::string rnd;
        void update(Blake2bHasher& h) const
        {
            h.update(user_pk_hex_ed);
            h.update(rnd);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<user_pk_hex_ed;
            b<<rnd;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>user_pk_hex_ed;
            b>>rnd;
        }

        static Base* construct()
        {
            return new GetUserStatusREQ();
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetUserStatusREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetUserStatusREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetUserStatusREQ();
    s->unpack2(b);
    return b;
}
