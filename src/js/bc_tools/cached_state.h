#pragma once
#include "REF.h"
#include <vector>
#include "root_contract.h"
struct cached_state: public Refcountable
{
    std::vector<REF_getter<bc_node>> nodes;
    REF_getter<bc_values> values_granule;
    REF_getter<bc_user_state> user_state_granule;
};