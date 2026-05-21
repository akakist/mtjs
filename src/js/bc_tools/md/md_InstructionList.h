#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct InstructionList: public Base
    {
        
        static Base* construct()
        {
            return new InstructionList();
        }
        InstructionList():Base(msgid::InstructionList)
        {

        }
        std::vector<REF_getter<MsgData::Base>> container;

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<container;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>container;
        }
        void update(Blake2bHasher& h) const
        {
            MUTEX_INSPECTOR;
            for(auto& z: container)
            {
                z->update(h);
                // h.update(z);
            }
        }
    };


}

inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::InstructionList> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::InstructionList> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::InstructionList();
    s->unpack2(b);
    return b;
}
