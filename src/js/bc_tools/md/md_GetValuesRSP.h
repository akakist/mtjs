#pragma once
#include "md_Base.h"
#include <map>
namespace MsgData
{
    struct GetValuesRSP: public Base
    {
        GetValuesRSP():Base(msgid::GetValuesRSP) {}
        bool found = false;
        std::map<std::string, uint64_t> fees;
        void update(Blake2bHasher& h) const
        {
            h.update(found ? "1" : "0");
            for(auto& z: fees) { h.update(z.first); h.update(std::to_string(z.second)); }
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<found<<fees;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>found>>fees;
        }
        static Base* construct() { return new GetValuesRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetValuesRSP> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetValuesRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetValuesRSP();
    s->unpack2(b);
    return b;
}
