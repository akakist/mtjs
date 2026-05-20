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
        std::vector<REF_getter<MsgData::Base>> instructions;

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<instructions.size();
            for(auto& z: instructions)
            {
                z->pack(b);
            }
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            int n=b.get_PN();
            printf("!!!!!!!!!!!!!!!! n=%d \n", n);
            for(int i=0;i<n;i++)
            {
                MUTEX_INSPECTOR;
                auto m=b.get_PN();
                REF_getter<MsgData::Base> msg=msgFactory.create(m);
                msg->unpack(b);
                instructions.push_back(msg);

            }
        }
        void update(Blake2bHasher& h) const
        {
            MUTEX_INSPECTOR;
            for(auto& z: instructions)
            {
                z->update(h);
                // h.update(z);
            }
        }
    };


}
