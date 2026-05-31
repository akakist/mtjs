#pragma once
#include <optional>
#include "msg.h"
#include "msg_tx.h"
#include "root_contract.h"
#include "t_params.h"

namespace TR {
    std::optional<std::string> execute_mint(const yyjson::Value &params, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_transfer(const yyjson::Value &params, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_create_node(const yyjson::Value &params, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_update_node(const yyjson::Value &params, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);

}
