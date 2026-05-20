#pragma once
#include "md_Base.h"
#include "md_TX.h"
namespace MsgData
{
    struct GetTransactionRSP: public Base
    {
        
        static Base* construct()
        {
            return new GetTransactionRSP();
        }
        GetTransactionRSP():Base(msgid::GetTransactionRSP)
        {
            
        }
        GetTransactionRSP(inBuffer &in):Base(msgid::GetTransactionRSP)
        {
            unpack(in);
        }

        std::vector<REF_getter<TX> >  trs;
        void update(Blake2bHasher& h) const
        {
            for(auto& z:trs )
                z->hash(h);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<trs.size();
            for(auto& z: trs)
            {
                z->pack(b);
            }
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            int n=b.get_PN();
            for(int i=0;i<n;i++)
            {
                REF_getter<MsgData::TX> tx=new TX;
                tx->unpack2(b);
                trs.push_back(tx);
            }
        }

    };


}
