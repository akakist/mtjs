// bc_tools/bc_state.h
#pragma once
#include "cellable.h"
#include "i128.h"

struct bc_state: public data_base {
    bc_state(Cellable *p): data_base(hsh::bc_state,p,0,-1) {}
    i128 total_supply;         // 16
    i128 total_stake;          // 16
    int64_t account_count = 0; // 8
    int64_t node_count = 0;    // 8
    int64_t contract_count = 0;// 8
    int64_t tx_count = 0;      // 8
    int64_t block_height = 0;  // 8
    int64_t genesis_time = 0;  // 8
    int64_t chain_id = 0;      // 8
    // size() = size_() + 16+16 + 8*7 = ~88
    size_t size() const {
        return size_() + sizeof(i128)*2 + sizeof(int64_t)*7;
    }
    void pack(outBuffer& o) const final {
        data_base::pack(o);
        o<<1;
        o<<total_supply<<total_stake
         <<account_count<<node_count<<contract_count
         <<tx_count<<block_height<<genesis_time<<chain_id;
    }
    void unpack(inBuffer& o) final {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>total_supply>>total_stake
         >>account_count>>node_count>>contract_count
         >>tx_count>>block_height>>genesis_time>>chain_id;
    }
};
