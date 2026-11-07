#pragma once
#include "md_Base.h"
#include "CONTRACT_id.h"
namespace MsgData
{
    struct CallViewREQ: public Base
    {
        CallViewREQ():Base(msgid::CallViewREQ) {}
        CONTRACT_id contract;
        std::string view;          // имя view
        std::string params_json;   // JSON-строка (nlohmann — на стороне GrainReader)
        int64_t timeout_sec = 5;
        void update(Blake2bHasher& h) const
        {
            h.update(contract.container);
            h.update(view);
            h.update(params_json);
            h.update(std::to_string(timeout_sec));
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<contract<<view<<params_json<<timeout_sec;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>contract>>view>>params_json>>timeout_sec;
        }
        static Base* construct() { return new CallViewREQ(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::CallViewREQ> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::CallViewREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::CallViewREQ();
    s->unpack2(b);
    return b;
}
