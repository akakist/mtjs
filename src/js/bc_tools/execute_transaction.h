#pragma once
#include "t_params.h"
#include <string>
#include <vector>
// void
void execute_transaction(const THASH_id& tx_id, t_params &t, const std::string &senderAddress,
                         const nlohmann::json &tx, const REF_getter<fee_calcer> &by);

// std::optional<std::string> TR::execute_mint(const nlohmann::json &params, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
