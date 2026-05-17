#pragma once
#include "t_params.h"
#include <string>
#include <vector>
void
execute_transaction(const THASH_id&  tx_id, t_params& t, const std::string& senderAddress, const std::vector<std::string>& payloads,const REF_getter<fee_calcer> &by);
