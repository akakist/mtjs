#pragma once
#include "md_Base.h"
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
        // yyjson::Document j;
    public:
        std::string tx_body;
        // THASH_id hash;
        std::string pk_ed_bin;
        std::string sig_ed_bin;
        BigInt nonce;

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<tx_body<<pk_ed_bin<<sig_ed_bin<<nonce;

        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>tx_body>>pk_ed_bin>>sig_ed_bin>>nonce;
        }
        void update(Blake2bHasher &h) const
        {
            MUTEX_INSPECTOR;
            // throw CommonError("unimpl");
            h.update(tx_body);
            h.update(pk_ed_bin);
            h.update(nonce.toString());
        }
        bool verify()
        {
            // auto &tx=j["tx"];
            auto h=getHash();
            // auto sign=base16::decode(j["sign"].get<std::string>());
            // auto pk=base16::decode(j["pk"].get<std::string>());
            return verify_ed_pk(pk_ed_bin,sig_ed_bin,h);
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

