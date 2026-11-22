#pragma once
#include "md_Base.h"
#include "rsp_status.h"
#include <map>
#include <string>
namespace MsgData
{
    // Список всех нод: name -> "ip:port" (RPC-адрес из bc_node.ip).
    struct GetNodesRSP: public Base, public rsp_status
    {
        GetNodesRSP():Base(msgid::GetNodesRSP) { setError(msgcode::NOT_FOUND, "not found"); }
        std::map<std::string, std::string> nodes;
        void update(Blake2bHasher& h) const
        {
            update_status(h);
            for (auto& z : nodes) { h.update(z.first); h.update(z.second); }
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            pack_status(b);
            b << nodes;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            unpack_status(b);
            b >> nodes;
        }
        static Base* construct() { return new GetNodesRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetNodesRSP> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetNodesRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetNodesRSP();
    s->unpack2(b);
    return b;
}
