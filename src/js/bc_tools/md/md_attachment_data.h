#pragma once
#include "md_Base.h"
#include "md_TX.h"
#include "bigint.h"
#include "ADDRESS_id.h"
struct EmitNode {
    std::vector<std::pair<std::string,std::string>> emits;                    // события на этом уровне
    std::map<std::string, EmitNode> children;          // дочерние узлы
    void update(Blake2bHasher &b) const
    {
        for(auto &z:emits)
        {
            b.update(z.first);
            b.update(z.second);
        }
        for(auto &z:children)
        {
            b.update(z.first);
            z.second.update(b);
        }
    }
};
inline outBuffer & operator<< (outBuffer& b,const EmitNode &s)
{
    b<<s.emits;
    b<<s.children;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  EmitNode &s)
{
    b>>s.emits;
    b>>s.children;
    return b;
}

namespace MsgData
{
    struct attachment_data: public Base
    {
        attachment_data():Base(msgid::attachment_data)
        {

        }
        EmitNode blockRoot;
        std::map<ADDRESS_id,BigInt> fees;
        std::map<NODE_id,BigInt> rewards;
        // std::vector<std::string> emitted_events;

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<fees<<rewards<<blockRoot;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>fees>>rewards>>blockRoot;
        }

        void update(Blake2bHasher &h) const
        {
            blockRoot.update(h);
            for(auto &z: fees)
            {
                h.update(z.first.addr);
                h.update(z.second.toString());
            }
            for(auto &z: rewards)
            {
                h.update(z.first.container);
                h.update(z.second.toString());
            }
        }
    };


}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::attachment_data> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::attachment_data> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::attachment_data();
    s->unpack2(b);
    return b;
}


