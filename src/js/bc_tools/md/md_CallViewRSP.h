#pragma once
#include "md_Base.h"
#include "rsp_status.h"
namespace MsgData
{
    struct CallViewRSP: public Base, public rsp_status
    {
        CallViewRSP():Base(msgid::CallViewRSP) {}
        bool error = false;        // view бросил исключение
        std::string result_json;   // JSON-строка результата (или errstr)
        void update(Blake2bHasher& h) const
        {
            update_status(h);
            h.update(error ? "1" : "0");
            h.update(result_json);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            pack_status(b);
            b<<error<<result_json;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            unpack_status(b);
            b>>error>>result_json;
        }
        static Base* construct() { return new CallViewRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::CallViewRSP> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::CallViewRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::CallViewRSP();
    s->unpack2(b);
    return b;
}
