#pragma once

namespace hsh
{
    enum hashers
    {
        bc_contract,
        bc_address_state,
        bc_node,
        bc_nodelist,
        bc_values,
        // bc_epoch,
        bc_granule,
        bc_votes,
        bc_contract_balance,
        bc_state,
        bc_activity,
        bc_deleted,   // тумбстоун: время удаления = last_block_timestamp
        HSH_END
    };
}
