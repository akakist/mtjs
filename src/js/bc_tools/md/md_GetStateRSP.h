#pragma once
#include "md_Base.h"
#include <string>
namespace MsgData
{
    struct GetStateRSP: public Base
    {
        GetStateRSP():Base(msgid::GetStateRSP) {}
        bool found = false;
        std::string total_supply;   // i128::str()
        std::string total_stake;    // i128::str()
        int64_t account_count = 0;
        int64_t node_count = 0;
        int64_t contract_count = 0;
        int64_t tx_count = 0;
        int64_t block_height = 0;
        int64_t genesis_time = 0;
        int64_t chain_id = 0;
        void update(Blake2bHasher& h) const
        {
            h.update(found ? "1" : "0");
            h.update(total_supply); h.update(total_stake);
            h.update(std::to_string(account_count));
            h.update(std::to_string(node_count));
            h.update(std::to_string(contract_count));
            h.update(std::to_string(tx_count));
            h.update(std::to_string(block_height));
            h.update(std::to_string(genesis_time));
            h.update(std::to_string(chain_id));
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<found<<total_supply<<total_stake
             <<account_count<<node_count<<contract_count
             <<tx_count<<block_height<<genesis_time<<chain_id;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>found>>total_supply>>total_stake
             >>account_count>>node_count>>contract_count
             >>tx_count>>block_height>>genesis_time>>chain_id;
        }
        static Base* construct() { return new GetStateRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetStateRSP> &s)
{ b<<1; s->pack(b); return b; }
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetStateRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetStateRSP();
    s->unpack2(b);
    return b;
}