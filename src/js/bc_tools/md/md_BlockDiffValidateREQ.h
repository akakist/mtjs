#pragma once
#include "md_Base.h"

// #include "md_BlockInfo.h"
#include "md_BlockAcceptedREQ.h"
#include "blst_cp.h"
#include "md_attachment_data.h"
#include "md_BlockDBStore.h"
namespace MsgData
{
    struct BlockDiffValidateREQ: public Base
    {

        BlockDiffValidateREQ(): Base(msgid::BlockDiffValidateREQ), blockAcceptedREQ(new BlockAcceptedREQ)
        {

        }
        static Base* construct()
        {
            return new BlockDiffValidateREQ();
        }
        // REFBlockStore
        REF_getter<BlockAcceptedREQ> blockAcceptedREQ;
        void update(Blake2bHasher& h) const
        {
    MUTEX_INSPECTOR;
            blockAcceptedREQ->update(h);
        }
        void pack(outBuffer& b) const final
        {
    MUTEX_INSPECTOR;
            Base::pack(b);
            b<<blockAcceptedREQ;
        }
        void unpack(inBuffer& b) final
        {
    MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>blockAcceptedREQ;
        }
        size_t size()
        {
            size_t sz=0;
            if(blockAcceptedREQ.valid())
                sz+=blockAcceptedREQ->size();


            return sz;
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockDiffValidateREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockDiffValidateREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockDiffValidateREQ();
    s->unpack2(b);
    return b;
}