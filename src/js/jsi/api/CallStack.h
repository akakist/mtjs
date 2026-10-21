#pragma once
#include <vector>
#include "REF.h"
struct CallFrame;
struct CallStack : public Refcountable {
    CallStack() : Refcountable("CallStack") {}
    std::vector<REF_getter<CallFrame>> call_stack;
};
