#pragma once
#include "md_Base.h"
#include "md_InstructionList.h"
namespace MsgData
{
    struct TX: public Base
    {
        
        static Base* construct()
        {
            return new TX();
        }
        TX():Base(msgid::TX), instructions(new InstructionList())
        {

        }
        REF_getter<InstructionList> instructions;
        std::string user_pk_ed;
        std::string sig_ed;
        BigInt nonce;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<user_pk_ed<<sig_ed<<nonce;
            instructions->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>user_pk_ed>>sig_ed>>nonce;
            instructions->unpack2(b);
        }
        void update(Blake2bHasher &h) const
        {
            instructions->update(h);
            h.update(user_pk_ed);
            h.update(nonce.toString());
        }
        void sign(const std::string& sk)
        {
            Blake2bHasher h;
            update(h);
            sig_ed=sign_ed(sk,h.final());
        }
        bool verify()
        {
            Blake2bHasher h;
            update(h);
            return verify_ed_pk(user_pk_ed,sig_ed,h.final());
        }
    };


}
