#pragma once
#include "md_Base.h"
#include "md_BlockDBStore.h"

namespace MsgData
{
    struct GetSavedBlocksRSP: public Base
    {

        static Base* construct()
        {
            return new GetSavedBlocksRSP();
        }
        GetSavedBlocksRSP():Base(msgid::GetSavedBlocksRSP)
        {

        }
        std::vector<REF_getter<MsgData::BlockDBStore>> blocks_ZZ;
        // BigInt lastEpoch;
        BLOCK_id last_prev_root_hash;
        void update(Blake2bHasher& h) const
        {
            throw CommonError("unimp");
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<blocks_ZZ;
            b<<last_prev_root_hash;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>blocks_ZZ;
            b>>last_prev_root_hash;
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetSavedBlocksRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetSavedBlocksRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetSavedBlocksRSP();
    s->unpack2(b);
    return b;
}
