#pragma once
#include "md_Base.h"
#include "md_LeaderCertificate.h"
namespace MsgData
{
    struct GetUserStatusRSP: public Base
    {

        GetUserStatusRSP():Base(msgid::GetUserStatusRSP)
        {
        }
        BigInt balance;
        BigInt nonce;
        void update(Blake2bHasher& h) const
        {
            h.update(balance.toString());
            h.update(nonce.toString());
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<balance<<nonce;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>balance>>nonce;
        }

        static Base* construct()
        {
            return new GetUserStatusRSP();
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetUserStatusRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetUserStatusRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetUserStatusRSP();
    s->unpack2(b);
    return b;
}
