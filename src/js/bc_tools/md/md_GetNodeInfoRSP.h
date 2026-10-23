#pragma once
#include "md_Base.h"
#include "NODE_id.h"
#include "ADDRESS_id.h"
#include <map>
namespace MsgData
{
    struct GetNodeInfoRSP: public Base
    {
        GetNodeInfoRSP():Base(msgid::GetNodeInfoRSP) {}
        bool found = false;
        NODE_id name;
        ADDRESS_id owner;
        std::string ed_pk;
        std::string bls_pk;
        std::string ip;
        bool enabled = false;
        int64_t missed_rounds = 0;
        uint64_t full_stake = 0;
        std::map<ADDRESS_id, uint64_t> stakes;
        void update(Blake2bHasher& h) const
        {
            h.update(found ? "1" : "0");
            h.update(name.container);
            h.update(owner.addr);
            h.update(ed_pk); h.update(bls_pk); h.update(ip);
            h.update(enabled ? "1" : "0");
            h.update(std::to_string(missed_rounds));
            h.update(std::to_string(full_stake));
            for(auto& z: stakes) { h.update(z.first.addr); h.update(std::to_string(z.second)); }
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<found<<name<<owner<<ed_pk<<bls_pk<<ip<<enabled<<missed_rounds<<full_stake<<stakes;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>found>>name>>owner>>ed_pk>>bls_pk>>ip>>enabled>>missed_rounds>>full_stake>>stakes;
        }
        static Base* construct() { return new GetNodeInfoRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetNodeInfoRSP> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetNodeInfoRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetNodeInfoRSP();
    s->unpack2(b);
    return b;
}
