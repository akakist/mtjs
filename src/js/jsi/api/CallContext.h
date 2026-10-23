#pragma once
#include "REF.h"
#include "jsiMsg.h"
#include "Rollback.h"
#include "jsiEvent.h"
#include <vector>
struct ContractContext;
struct CallStack;
struct ContractHost;
struct t_params;
struct b_params;

struct CallContext {
    REF_getter<ContractContext> contract;
    bool in_view = false;
    jsiMsg msg;                          // для test_runner/совместимости; в ноде t != nullptr
    int depth = 0;
    REF_getter<CallStack> call_stack;
    REF_getter<ContractHost> host;
    Rollback * roll = nullptr;
    std::vector<jsiEvent> emits;

    // источник истины для msg-геттеров (root/nested — один и тот же root)
    t_params* t = nullptr;
    b_params* b = nullptr;
};



