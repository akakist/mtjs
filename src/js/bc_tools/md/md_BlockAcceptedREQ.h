#pragma once
#include "md_Base.h"

#include "md_BlockInfo.h"
#include "blst_cp.h"
namespace MsgData
{
    struct BlockAcceptedREQ: public Base
    {

        BlockAcceptedREQ()
            : Base(msgid::BlockAcceptedREQ),
            blockInfo(new BlockInfo)
        {
        }

        static Base* construct()
        {
            return new BlockAcceptedREQ();
        }
        REF_getter<BlockInfo> blockInfo;
        std::vector<NODE_id> node_validators;
        blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const;
        void pack(outBuffer& b) const final;
        void unpack(inBuffer& b) final;
        size_t size()
        {
            size_t sz=0;
            if(blockInfo.valid())
                sz+=blockInfo->size();
            
            for(auto &z: node_validators)
                sz+=z.container.size();
            sz+=agg_sig.serialize().size();

            return sz;
        }
    };

}
inline void MsgData::BlockAcceptedREQ::pack(outBuffer &b) const
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::pack(b);
    b << blockInfo;
    b << node_validators << agg_sig;
    XPASS;
}
inline void MsgData::BlockAcceptedREQ::unpack(inBuffer &b)
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::unpack(b);
    b >> blockInfo;
    b >> node_validators >> agg_sig;
    XPASS;
}

inline void MsgData::BlockAcceptedREQ::update(Blake2bHasher &h) const
{
    blockInfo->update(h);
    for (auto &z : node_validators)
    {
        h.update(z.container);
    }
}

inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockAcceptedREQ> &s)
{
    MUTEX_INSPECTOR;
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockAcceptedREQ> &s)
{
    MUTEX_INSPECTOR;
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockAcceptedREQ();
    s->unpack2(b);
    return b;
}