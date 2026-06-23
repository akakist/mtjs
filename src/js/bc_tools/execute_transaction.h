#pragma once
#include "t_params.h"
#include <string>
void execute_transaction(const THASH_id &tx_id, t_params &t, const ADDRESS_id &senderAddress,
                         const std::string &tx_cmds, const REF_getter<fee_calcer> &by, const EPOCH_id& epoch);

