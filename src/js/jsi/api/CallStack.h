#pragma once
#include <vector>
#include "REF.h"
#include "CallFrame.h"
struct CallStack  {
    CallStack()  {}
    std::vector<REF_getter<CallFrame>> call_stack;
};
