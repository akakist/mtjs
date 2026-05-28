#pragma once
#include "md_Base.h"
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
        }
        void update(Blake2bHasher &h) const
        {
            MUTEX_INSPECTOR;
            throw CommonError("unimpl");
        }
        bool verify()
        {
            auto &tx=j["tx"];
            auto h=blake2b_hash(tx.dump());
            auto sign=base62::decode(j["sign"].get<std::string>());
            auto pk=base62::decode(j["pk"].get<std::string>());
            return verify_ed_pk(pk,sign,h);
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

