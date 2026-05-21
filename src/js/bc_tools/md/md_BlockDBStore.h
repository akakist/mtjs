#pragma once
#include "md_Base.h"
#include "md_attachment_data.h"
#include "md_BlockAcceptedREQ.h"
namespace MsgData
{
    struct BlockDBStore: public Base
    {
        
        BlockDBStore():Base(msgid::BlockDBStore), 
        att_data(new attachment_data),
        block_accepted_req(new BlockAcceptedREQ())
        {

        }
        BigInt epoch;
        REF_getter<attachment_data> att_data;
        REF_getter<BlockAcceptedREQ> block_accepted_req;
        void update(Blake2bHasher& h) const
        {
            h.update(epoch.toString());
            att_data->update(h);
            block_accepted_req->update(h);
        }
        void pack(outBuffer& b) const final
        {
            XTRY;
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<epoch;
            b<<att_data;
            b<<block_accepted_req;
            XPASS;
        }
        void unpack(inBuffer& b) final
        {
            XTRY;
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>epoch;
            b>>att_data;
            b>>block_accepted_req;
            XPASS;
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockDBStore> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockDBStore> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockDBStore();
    s->unpack2(b);
    return b;
}
