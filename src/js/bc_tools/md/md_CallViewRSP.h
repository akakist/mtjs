#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct CallViewRSP: public Base
    {
        CallViewRSP():Base(msgid::CallViewRSP) {}
        bool found = false;        // контракт/view найден
        bool error = false;        // view бросил исключение
        std::string result_json;   // JSON-строка результата (или errstr)
        void update(Blake2bHasher& h) const
        {
            h.update(found ? "1" : "0");
            h.update(error ? "1" : "0");
            h.update(result_json);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<found<<error<<result_json;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>found>>error>>result_json;
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
