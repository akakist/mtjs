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

        BlockDiffValidateREQ(const REF_getter<BlockAcceptedREQ> & ba,
        const std::vector<THASH_id> & _tx,
        const std::map<std::string, std::string> &_diffs,
        const REF_getter<MsgData::attachment_data> &_att_data
    ): Base(msgid::BlockDiffValidateREQ), 
            blockAcceptedREQ(ba),
            tx_hashes(_tx),
            diffs(_diffs),
            att_data(_att_data)
        {

        }
        BlockDiffValidateREQ(): Base(msgid::BlockDiffValidateREQ){};
        static Base* construct()
        {
            return new BlockDiffValidateREQ();
        }
        REF_getter<BlockAcceptedREQ> blockAcceptedREQ;

        std::vector<THASH_id> tx_hashes;
        std::map<std::string, std::string> diffs;
        REF_getter<MsgData::attachment_data> att_data;
        void update(Blake2bHasher& h) const
        {
    MUTEX_INSPECTOR;
            blockAcceptedREQ->update(h);
            for(auto& t: tx_hashes)
            {
                h.update(t.container);
            }
            for(auto& t: diffs)
            {
                h.update(t.first);
                h.update(t.second);
            }
            att_data->update(h);
        }
        void pack(outBuffer& b) const final
        {
    MUTEX_INSPECTOR;
            Base::pack(b);
            b << blockAcceptedREQ;
            b << tx_hashes << diffs << att_data;
        }
        void unpack(inBuffer& b) final
        {
    MUTEX_INSPECTOR;
            Base::unpack(b);
            b >> blockAcceptedREQ;
            b >> tx_hashes >> diffs >> att_data;
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