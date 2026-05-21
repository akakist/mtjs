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
            lc->update(h);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<lc;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>lc;
        }

        static Base* construct()
        {
            return new GetTransactionREQ();
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetTransactionREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetTransactionREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetTransactionREQ();
    s->unpack2(b);
    return b;
}
