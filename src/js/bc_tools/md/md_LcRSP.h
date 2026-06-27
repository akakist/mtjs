#pragma once
#include "msg.h"
#include "bigint.h"
#include "BLOCK_id.h"
#include "NODE_id.h"
#include "md_Base.h"
namespace MsgData
{
    struct LcRSP: public Base
    {

        LcRSP():Base(msgid::LcRSP)
        {

        }
        LcRSP(const std::string& _prev_lc):Base(msgid::LcRSP),
            prev_lc(_prev_lc)
        {
        }
        std::string prev_lc;
        void update(Blake2bHasher& h) const
        {
            h.update(prev_lc);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<prev_lc;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>prev_lc;
        }
        static Base* construct()
        {
            return new LcRSP();
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::LcRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::LcRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::LcRSP();
    s->unpack2(b);
    return b;
}
