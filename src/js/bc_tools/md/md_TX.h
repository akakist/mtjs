#pragma once
#include "md_Base.h"
#include "s_ed.h"
#include "bigint.h"
#include "yyjson.h"
#include <optional>
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
        ~TX()
        {
            if(doc)
                yyjson_doc_free(doc);
        }
    private:
        // yyjson::Document j;
    public:
        std::string tx_body;
        std::string pk_ed_bin;
        std::string sig_ed_bin;
        // uint64_t nonce;
        // BigInt gasLimit;
        // BigInt value;
        yyjson_doc *doc=nullptr;
        // yyjson::Value root;

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<tx_body<<pk_ed_bin<<sig_ed_bin;
            // <<nonce<<gasLimit<<value;

        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>tx_body>>pk_ed_bin>>sig_ed_bin;

            doc=yyjson_read(tx_body.data(),tx_body.size(),0);
            // doc.read(tx_body);
            // root=doc.root();
            // >>nonce>>gasLimit>>value;
        }
        void update(Blake2bHasher &h) const
        {
            MUTEX_INSPECTOR;
            // throw CommonError("unimpl");
            h.update(tx_body);
            h.update(pk_ed_bin);
            // h.update(std::to_string(nonce));
            // h.update(gasLimit.toString());
            // h.update(value.toString());
        }
        std::optional<std::string> getNonce(uint64_t & nonce)
        {
            if(!doc)
                return "yyjson: !doc";
            auto root=yyjson_doc_get_root(doc);
            if(!root)
                return "yyjson: !root";
            auto v=yyjson_obj_get(root,"nonce");
            if(!v)
                return "yyjson: omitted nonce";
            if(yyjson_is_num(v))
            {
                nonce=yyjson_get_uint(v);
                return std::nullopt;
            }
            else if(yyjson_is_str(v))
            {
                nonce=atoll(yyjson_get_str(v));
                return std::nullopt;
            }
            else return "yyjson: nonce must be string or num";
            return std::nullopt;
        }
        std::optional<std::string> getGasLimit(BigInt & gasLimit)
        {
            if(!doc)
                return "yyjson: !doc";
            auto root=yyjson_doc_get_root(doc);
            if(!root)
                return "yyjson: !root";
            auto v=yyjson_obj_get(root,"gasLimit");
            if(!v)
                return "yyjson: omitted gasLimit";
            if(yyjson_is_num(v))
            {
                gasLimit=yyjson_get_uint(v);
                return std::nullopt;
            }
            else if(yyjson_is_str(v))
            {
                gasLimit.from_string(yyjson_get_str(v));
                return std::nullopt;
            }
            else return "yyjson: gasLimit must be string or num";
            return std::nullopt;
        }
        std::optional<std::string> getGasPrice(BigInt & gasPrice)
        {
            if(!doc)
                return "yyjson: !doc";
            auto root=yyjson_doc_get_root(doc);
            if(!root)
                return "yyjson: !root";
            auto v=yyjson_obj_get(root,"gasPrice");
            if(!v)
                return "yyjson: omitted gasPrice";
            if(yyjson_is_num(v))
            {
                gasPrice=yyjson_get_uint(v);
                return std::nullopt;
            }
            else if(yyjson_is_str(v))
            {
                gasPrice.from_string(yyjson_get_str(v));
                return std::nullopt;
            }
            else return "yyjson: gasPrice must be string or num";
            return std::nullopt;
        }
        std::optional<std::string> getValue(BigInt & value)
        {
            if(!doc)
                return "yyjson: !doc";
            auto root=yyjson_doc_get_root(doc);
            if(!root)
                return "yyjson: !root";
            auto v=yyjson_obj_get(root,"value");
            if(!v)
                return "yyjson: omitted value";
            if(yyjson_is_num(v))
            {
                value=yyjson_get_uint(v);
                return std::nullopt;
            }
            else if(yyjson_is_str(v))
            {
                value.from_string(yyjson_get_str(v));
                return std::nullopt;
            }
            else return "yyjson: value must be string or num";
            return std::nullopt;
        }
        bool verify()
        {
            auto h=getHash();
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

