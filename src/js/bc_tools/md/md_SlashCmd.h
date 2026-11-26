#pragma once
#include "md_Base.h"
#include "NODE_id.h"
#include "THASH_id.h"
#include "blst_cp.h"

namespace MsgData
{
    // Эвиденс двойной подписи: валидатор node подписал два РАЗНЫХ результата
    // (hash1 и hash2) одним и тем же BLS-ключом. Детерминированно проверяется
    // в execute_block (два verify) и ведёт к полному сжиганию стейка.
    struct SlashCmd: public Base
    {
        SlashCmd():Base(msgid::SlashCmd) {}
        static Base* construct() { return new SlashCmd(); }

        NODE_id node;
        THASH_id hash1, hash2;
        blst_cpp::Signature sig1, sig2;

        void update(Blake2bHasher& h) const
        {
            h.update(node.container);
            h.update(hash1.container);
            h.update(hash2.container);
            h.update(sig1.serialize());
            h.update(sig2.serialize());
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<node<<hash1<<hash2<<sig1<<sig2;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>node>>hash1>>hash2>>sig1>>sig2;
        }
    };
}

inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::SlashCmd> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::SlashCmd> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::SlashCmd();
    s->unpack2(b);
    return b;
}
