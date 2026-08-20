#pragma once
#include "md_Base.h"

#include "md_BlockInfo.h"
#include "md_TX.h"
#include "blst_cp.h"
#include "NODE_id.h"
namespace MsgData
{
    struct ValidateBlockRSP: public Base
    {

        static Base* construct()
        {
            return new ValidateBlockRSP();
        }
        ValidateBlockRSP(): Base(msgid::ValidateBlockRSP), blockInfo(new BlockInfo())
        {

        }
        REF_getter<BlockInfo> blockInfo;
        blst_cpp::Signature sig;
        NODE_id node_validator;
        size_t size()
        {
            size_t sz=0;
            sz+=node_validator.container.size();
            sz+=sig.serialize().size();
            if(blockInfo.valid())
                sz+=blockInfo->size();
            return sz;
        }
        void dump(nlohmann::json& j)
        {
            j["sig"]=base16::encode(sig.serialize());
            j["node_validator"]=node_validator.container;
            blockInfo->dump(j["blockInfo"]);
        }
        void update(Blake2bHasher& h) const
        {
            blockInfo->update(h);
            h.update(node_validator.container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<blockInfo;
            b<<sig;
            b<<node_validator;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>blockInfo;
            b>>sig;
            b>>node_validator;
        }
        void sign(const blst_cpp::SecretKey &sk)
        {
            sig.sign(sk, blake2b_hash(blockInfo->getBuffer()).container);
        }
        bool verify(const blst_cpp::PublicKey &pk) const
        {
            return sig.verify(pk, blake2b_hash(blockInfo->getBuffer()).container);
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::ValidateBlockRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::ValidateBlockRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::ValidateBlockRSP();
    s->unpack2(b);
    return b;
}
