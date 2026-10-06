#pragma once
#include "md_Base.h"

// #include "md_BlockInfo.h"
#include "md_BlockAcceptedREQ.h"
#include "blst_cp.h"
namespace MsgData
{
    struct BlockDiffValidateRSP: public Base
    {

        BlockDiffValidateRSP(): Base(msgid::BlockDiffValidateRSP), payload_blockAcceptedREQ(new BlockAcceptedREQ)
        {

        }
        static Base* construct()
        {
            return new BlockDiffValidateRSP();
        }
        REF_getter<BlockAcceptedREQ> payload_blockAcceptedREQ;
        blst_cpp::Signature sig;
        NODE_id node_validator;

        // REF_getter<BlockAcceptedREQ> blockValidatedREQ;
        // std::map<std::string,std::string> diffs;
        // std::vector<NODE_id> node_validators;
        // blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const
        {
            payload_blockAcceptedREQ->update(h);
            h.update(sig.serialize());
            h.update(node_validator.container);

        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<payload_blockAcceptedREQ;
            b<<sig;
            b<<node_validator;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>payload_blockAcceptedREQ;
            b>>sig;
            b>>node_validator;
        }
        size_t size()
        {
            size_t sz=0;
            if(payload_blockAcceptedREQ.valid())
                sz+=payload_blockAcceptedREQ->size();

            sz+=sig.serialize().size();
            sz+=node_validator.container.size();
            return sz;
        }
        bool verify(const blst_cpp::PublicKey &pk) const
        {
            return sig.verify(pk, blake2b_hash(payload_blockAcceptedREQ->getBuffer()).container);
        }
        void sign(const blst_cpp::SecretKey &sk)
        {
            sig.sign(sk, blake2b_hash(payload_blockAcceptedREQ->getBuffer()).container);
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockDiffValidateRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockDiffValidateRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockDiffValidateRSP();
    s->unpack2(b);
    return b;
}