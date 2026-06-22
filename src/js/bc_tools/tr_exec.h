#pragma once
#include <optional>
#include "t_params.h"
#include "ADDRESS_id.h"

namespace TR {
    std::optional<std::string> execute_mint(const yyjson::Value &params, t_params & t,const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_transfer(const yyjson::Value &params, t_params & t,const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_node_create(const yyjson::Value &params, t_params & t,const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_node_update(const yyjson::Value &params, t_params & t,const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_node_stake(const yyjson::Value &params, t_params & t,const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_unstake_node(const yyjson::Value &params, t_params & t,const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_node_enable(const yyjson::Value &params, t_params & t,const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_contract_deploy(const yyjson::Value &params, t_params & t,const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);
    std::optional<std::string> execute_contract_update(const yyjson::Value &params, t_params & t,const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId);

}
