#pragma once
#include "md_Base.h"

#include "md_BlockInfo.h"
#include "md_BlockAcceptedREQ.h"
#include "md_TX.h"
#include "blst_cp.h"
#include "NODE_id.h"
#include "md_BlockDBStore.h"
#include "md_attachment_data.h"
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
        /// TODO - нужно выкинуть контент tx_hashes att_data  diffs. deleted - нужен ли, у нас схема удаления сейчас работает без этого вроде
        REF_getter<BlockInfo> blockInfo;
        REF_getter<attachment_data> att_data;
        blst_cpp::Signature sig;
        NODE_id node_validator;

        int round;
        int subblock;
        /// куда лидеру положить голос: false — accept (состояние ДО слэша),
        /// true — slash (состояние ПОСЛЕ слэша).
        bool slash=false;

        void update(Blake2bHasher& h) const
        {
            blockInfo->update(h);
            h.update(sig.serialize());
            h.update(node_validator.container);

            att_data->update(h);

            h.update(std::to_string(round));
            h.update(std::to_string(subblock));
            h.update(slash ? "1" : "0");
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<blockInfo;
            b<<sig;
            b<<node_validator<<att_data;
            b<<round<<subblock<<slash;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>blockInfo;
            b>>sig;
            b>>node_validator>>att_data;
            b>>round>>subblock>>slash;
        }
        void sign(const blst_cpp::SecretKey &sk)
        {
            sig.sign(sk, blockInfo->getHash().container);
        }
        bool verify(const blst_cpp::PublicKey &pk) const
        {
            return sig.verify(pk, blockInfo->getHash().container);
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
