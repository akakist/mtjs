#pragma once
struct ContractContext;
struct CallFrame : public Refcountable {
    CallFrame() : Refcountable("CallFrame") {}
    REF_getter<ContractContext> contract;
    std::string method;
    enum class Mode { Method, View, Query };
    Mode mode = Mode::Method;
};
