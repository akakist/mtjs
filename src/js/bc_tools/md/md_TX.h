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
            b<<instructions;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>user_pk_ed>>sig_ed>>nonce;
            b>>instructions;
        }
        void update(Blake2bHasher &h) const
        {
            instructions->update(h);
            h.update(user_pk_ed);
            h.update(nonce.toString());
        }
        void sign(const std::string& sk)
        {
            MUTEX_INSPECTOR;
            Blake2bHasher h;
            update(h);
            sig_ed=sign_ed(sk,h.final());
        }
        bool verify()
        {
            MUTEX_INSPECTOR;
            Blake2bHasher h;
            update(h);
            return verify_ed_pk(user_pk_ed,sig_ed,h.final());
        }
    };


}

inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::TX> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::TX> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::TX();
    s->unpack2(b);
    return b;
}

