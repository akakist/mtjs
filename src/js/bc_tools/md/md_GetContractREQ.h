#pragma once
#include "md_Base.h"
#include "CONTRACT_id.h"
namespace MsgData
{
    struct GetContractREQ: public Base
    {
        GetContractREQ():Base(msgid::GetContractREQ) {}
        CONTRACT_id contract;
        void update(Blake2bHasher& h) const
        {
            h.update(contract.container);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<contract;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>contract;
        }
        static Base* construct() { return new GetContractREQ(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetContractREQ> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetContractREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetContractREQ();
    s->unpack2(b);
    return b;
}