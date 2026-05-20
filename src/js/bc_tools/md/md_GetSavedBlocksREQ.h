#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct GetSavedBlocksREQ: public Base
    {
        
        GetSavedBlocksREQ():Base(msgid::GetSavedBlocksREQ)
        {

        }
        static Base* construct()
        {
            return new GetSavedBlocksREQ();
        }
        BigInt epoch;
        void update(Blake2bHasher& h) const
        {
            h.update(epoch.toString());
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<epoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>epoch;
        }
    };

}
