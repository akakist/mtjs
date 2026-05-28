#pragma once
#include "md_Base.h"
#include "md_InstructionList.h"
#include <nlohmann/json.hpp>
namespace MsgData
{
    struct TX: public Base
    {
        
        static Base* construct()
        {
            return new TX();
        }
        TX():Base(msgid::TX)
        // , instructions(new InstructionList())
        {

        }
        private:
        nlohmann::json j;
        public:
        THASH_id hash;

        void set_j(const nlohmann::json& _j)
        {
            j=_j;
            hash=blake2b_hash(j.dump());
        }
        const nlohmann::json& get_j() const
        {
            return j;
        }
        // REF_getter<InstructionList> instructions;
        // std::string user_pk_ed;
        // std::string sig_ed;
        // BigInt nonce;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<j.dump();
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            std::string s;
            b>>s;
            j=nlohmann::json::parse(s);
            hash=blake2b_hash(s);
            // b>>user_pk_ed>>sig_ed>>nonce;
            // b>>instructions;
        }
        void update(Blake2bHasher &h) const
        {
            // instructions->update(h);
            MUTEX_INSPECTOR;
            throw CommonError("unimpl");
            // h.update(j.dump());
            // h.update(nonce.toString());
        }
        bool verify()
        {
            auto &tx=j["tx"];
            auto h=blake2b_hash(tx.dump());
            auto sign=base62::decode(j["sign"].get<std::string>());
            auto pk=base62::decode(j["pk"].get<std::string>());
            return verify_ed_pk(pk,sign,h);
        }
        // void sign(const std::string& sk)
        // {
        //     MUTEX_INSPECTOR;
        //     Blake2bHasher h;
        //     update(h);
        //     sig_ed=sign_ed(sk,h.final());
        // }
        // bool verify()
        // {
        //     MUTEX_INSPECTOR;
        //     Blake2bHasher h;
        //     update(h);
        //     return verify_ed_pk(user_pk_ed,sig_ed,h.final());
        // }
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

