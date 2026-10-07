#pragma once
#include "md_Base.h"

// #include "md_BlockInfo.h"
#include "md_BlockAcceptedREQ.h"
#include "blst_cp.h"
namespace MsgData
{
    struct BlockDiffValidateRSP: public Base
    {

        BlockDiffValidateRSP(): Base(msgid::BlockDiffValidateRSP)
        // , blockAcceptedREQ(new BlockAcceptedREQ)
        {

        }
        static Base* construct()
        {
            return new BlockDiffValidateRSP();
        }
        blst_cpp::Signature sig;
        NODE_id node_validator;

        void update(Blake2bHasher& h) const
        {
            h.update(sig.serialize());
            h.update(node_validator.container);

        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<sig;
            b<<node_validator;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>sig;
            b>>node_validator;
        }
        size_t size()
        {
            size_t sz=0;

            sz+=sig.serialize().size();
            sz+=node_validator.container.size();
            return sz;
        }
        // bool verify(const blst_cpp::PublicKey &pk) const
        // {
        //     return sig.verify(pk, blockAcceptedREQ->getHash().container);
        // }
        // void sign(const blst_cpp::SecretKey &sk)
        // {
        //     sig.sign(sk, blockAcceptedREQ->getHash().container);
        // }

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