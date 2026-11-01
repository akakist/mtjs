#pragma once
#include <cstdint>
#pragma once
#include <cstdint>

struct FeesSnapshot {
    int64_t gas_per_token    = 1000;
    int64_t gas_per_byte     = 1;
    int64_t gas_per_jsi_step = 10;
    int64_t gas_per_hostcall = 500;
    int64_t gas_per_call     = 1000;
    int64_t min_tx_fee       = 1;
    int64_t gas_node_create  = 10000;
    int64_t gas_node_enable  = 5000;
    int64_t gas_node_stake   = 2000;
    int64_t gas_node_update  = 1000;
    int64_t gas_node_disable = 5000;
};
