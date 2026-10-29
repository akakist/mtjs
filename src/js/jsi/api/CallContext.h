#pragma once
#include "REF.h"
#include "jsiMsg.h"
#include "Rollback.h"
#include "jsiEvent.h"
#include <vector>
#include "IMsgSource.h"
struct ContractContext;
struct CallStack;
struct ContractHost;
// struct t_params;
// struct b_params;

struct CallContext {
    REF_getter<ContractContext> contract;
    bool in_view = false;
    int depth = 0;
    REF_getter<CallStack> call_stack;
    REF_getter<ContractHost> host;
    Rollback * roll = nullptr;
    std::vector<jsiEvent> emits;

    IMsgSource* msg_source = nullptr;  
};



