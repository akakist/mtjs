#pragma once
#include "md_Base.h"
#include "CONTRACT_id.h"
#include "CONTRACT_DATA_id.h"
namespace MsgData
{
    struct GetContractDataREQ: public Base
    {
        GetContractDataREQ():Base(msgid::GetContractDataREQ) {}
        CONTRACT_id contract;
        CONTRACT_DATA_id key;
        void update(Blake2bHasher& h) const
        {
            h.update(contract.container);
            h.update(key.container);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<contract<<key;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>contract>>key;
        }
        static Base* construct() { return new GetContractDataREQ(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetContractDataREQ> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetContractDataREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetContractDataREQ();
    s->unpack2(b);
    return b;
}
