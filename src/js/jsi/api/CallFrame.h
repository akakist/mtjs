#pragma once
struct ContractContext;
struct CallFrame : public Refcountable {
    CallFrame() : Refcountable("CallFrame") {}
    REF_getter<ContractContext> contract;
    std::string method;
    bool in_view = false;
};
