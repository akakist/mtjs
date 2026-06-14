#pragma once
#include "msg.h"
#include "bigint.h"
#include "BLOCK_id.h"
#include "NODE_id.h"
#include "md_Base.h"
namespace MsgData
{
    struct LcEnvelopeREQ: public Base
    {

        LcEnvelopeREQ():Base(msgid::LcEnvelopeREQ)
        {

        }
        LcEnvelopeREQ(const std::string& _msg, const std::string& _prev_lc):Base(msgid::LcEnvelopeREQ),
            msg(_msg),  prev_lc(_prev_lc)
        {
        }
        std::string msg;
        std::string prev_lc;
        void update(Blake2bHasher& h) const
        {
            h.update(msg);
            h.update(prev_lc);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<msg;
            b<<prev_lc;
            b<<prev_lc;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>msg;
            b>>prev_lc;
        }
        static Base* construct()
        {
            return new LcEnvelopeREQ();
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::LcEnvelopeREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::LcEnvelopeREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::LcEnvelopeREQ();
    s->unpack2(b);
    return b;
}
