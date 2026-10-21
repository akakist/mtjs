#pragma once
#include "REF.h"
#include "jsiMsg.h"
struct ContractContext;
struct CallStack;
struct ContractHost;
struct CallContext {
    REF_getter<ContractContext> contract;
    bool in_view = false;
    jsiMsg msg;
    int depth = 0;
    REF_getter<CallStack> call_stack;
    REF_getter<ContractHost> host;
};
