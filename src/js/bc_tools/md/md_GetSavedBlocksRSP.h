#pragma once
#include "md_Base.h"
namespace MsgData
#include "md_BlockDBStore.h"
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
        std::vector<std::pair<BigInt, REF_getter<MsgData::BlockDBStore>> > blocks_Z;
        BigInt lastEpoch;
        void update(Blake2bHasher& h) const
        {
            throw CommonError("unimp");
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<blocks_Z.size();
            for(auto &z: blocks_Z)
            {
                b<<z.first;
                z.second->pack(b);
            }
            b<<lastEpoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            int size=b.get_PN();
            for(int i=0;i<size;i++)            {
                BigInt epoch;
                b>>epoch;
                REF_getter<MsgData::BlockDBStore> block_db_store(new MsgData::BlockDBStore());
                block_db_store->unpack2(b);
                blocks_Z.emplace_back(epoch, block_db_store);
            }
            b>>lastEpoch;
        }
    };

}
