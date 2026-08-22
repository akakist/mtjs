#pragma once
#include "md_Base.h"

namespace MsgData
{
    struct GetUserNonceRSP: public Base
    {

        GetUserNonceRSP():Base(msgid::GetUserNonceRSP)
        {
        }
        uint64_t balance;
        uint64_t nonce;
        void update(Blake2bHasher& h) const
        {
            h.update(std::to_string(balance));
            h.update(std::to_string(nonce));
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b
            <<balance
            <<nonce;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b
            >>balance
            >>nonce;
        }

        static Base* construct()
        {
            return new GetUserNonceRSP();
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetUserNonceRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetUserNonceRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetUserNonceRSP();
    s->unpack2(b);
    return b;
}
