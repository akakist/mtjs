#pragma once
#include "msg.h"
#include "bigint.h"
#include "BLOCK_id.h"
#include "NODE_id.h"
#include "md_Base.h"
namespace MsgData
{
    struct LcREQ: public Base
    {

        LcREQ():Base(msgid::LcREQ)
        {

        }
        // LcREQ():Base(msgid::LcREQ),
        // {
        // }
        void update(Blake2bHasher& h) const
        {
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
        }
        static Base* construct()
        {
            return new LcREQ();
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::LcREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::LcREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::LcREQ();
    s->unpack2(b);
    return b;
}
