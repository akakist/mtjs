#pragma once
#include "md_Base.h"
#include <string>
#include "rsp_status.h"

namespace MsgData
{
    struct GetStateRSP: public Base, public rsp_status
    {
        GetStateRSP():Base(msgid::GetStateRSP) { setError(msgcode::NOT_FOUND, "not found"); }
        std::string total_supply;   // i128::str()
        std::string total_stake;    // i128::str()
        int64_t account_count = 0;
        int64_t node_count = 0;
        int64_t contract_count = 0;
        int64_t tx_count = 0;
        int64_t block_height = 0;
        int64_t genesis_time = 0;
        int64_t chain_id = 0;
        std::string root_hash;   // текущий корень состояния (Merkle root)
        void update(Blake2bHasher& h) const
        {
            update_status(h);
            h.update(total_supply); h.update(total_stake);
            h.update(std::to_string(account_count));
            h.update(std::to_string(node_count));
            h.update(std::to_string(contract_count));
            h.update(std::to_string(tx_count));
            h.update(std::to_string(block_height));
            h.update(std::to_string(genesis_time));
            h.update(std::to_string(chain_id));
            h.update(root_hash);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            pack_status(b);
            b<<total_supply<<total_stake
             <<account_count<<node_count<<contract_count
             <<tx_count<<block_height<<genesis_time<<chain_id<<root_hash;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            unpack_status(b);
            b>>total_supply>>total_stake
             >>account_count>>node_count>>contract_count
             >>tx_count>>block_height>>genesis_time>>chain_id>>root_hash;
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