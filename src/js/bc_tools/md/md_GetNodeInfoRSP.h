#pragma once
#include "md_Base.h"
#include "NODE_id.h"
#include "ADDRESS_id.h"
#include <map>
#include "rsp_status.h"
#include "i128.h"

namespace MsgData
{
    struct GetNodeInfoRSP: public Base, public rsp_status
    {
        GetNodeInfoRSP():Base(msgid::GetNodeInfoRSP) { setError(msgcode::NOT_FOUND, "not found"); }
        NODE_id name;
        ADDRESS_id owner;
        std::string ed_pk;
        std::string bls_pk;
        std::string ip;
        bool enabled = false;
        int64_t last_update = 0;
        i128 full_stake = i128(0);
        std::map<ADDRESS_id, int64_t> stakes;
        void update(Blake2bHasher& h) const
        {
            update_status(h);
            h.update(name.container);
            h.update(owner.addr);
            h.update(ed_pk); h.update(bls_pk); h.update(ip);
            h.update(enabled ? "1" : "0");
            h.update(std::to_string(last_update));
            h.update(full_stake.str());
            for(auto& z: stakes) { h.update(z.first.addr); h.update(std::to_string(z.second)); }
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            pack_status(b);
            b<<name<<owner<<ed_pk<<bls_pk<<ip<<enabled<<last_update<<full_stake<<stakes;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            unpack_status(b);
            b>>name>>owner>>ed_pk>>bls_pk>>ip>>enabled>>last_update>>full_stake>>stakes;
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
