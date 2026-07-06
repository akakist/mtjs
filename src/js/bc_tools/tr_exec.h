#pragma once
#include <optional>
#include "t_params.h"
#include "ADDRESS_id.h"

namespace TR {
    std::optional<std::string> execute_mint(yyjson_val *params, b_params & b,t_params &t, const REF_getter<fee_calcer>& by,  int seqId);
    std::optional<std::string> execute_transfer(yyjson_val *params, b_params & ,t_params &t, const REF_getter<fee_calcer>& by, int seqId);
    std::optional<std::string> execute_node_create(yyjson_val *params, b_params &,t_params &t, const REF_getter<fee_calcer>& by,  int seqId);
    std::optional<std::string> execute_node_update(yyjson_val *params, b_params &,t_params &t, const REF_getter<fee_calcer>& by,  int seqId);
    std::optional<std::string> execute_node_stake(yyjson_val *params, b_params &,t_params &t, const REF_getter<fee_calcer>& by, int seqId);
    std::optional<std::string> execute_unstake_node(yyjson_val *params, b_params &,t_params &t,const REF_getter<fee_calcer>& by, int seqId);
    std::optional<std::string> execute_node_enable(yyjson_val *params, b_params &,t_params &t,const REF_getter<fee_calcer>& by,  int seqId);
    std::optional<std::string> execute_contract_deploy(yyjson_val *params, b_params & ,t_params &t, const REF_getter<fee_calcer>& by, int seqId);
    std::optional<std::string> execute_contract_update(yyjson_val *params, b_params & ,t_params &t, const REF_getter<fee_calcer>& by, int seqId);

}
