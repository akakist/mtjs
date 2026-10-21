#pragma once
#include "REF.h"
#include <string>
struct EventSink : public Refcountable {
    EventSink() : Refcountable("EventSink") {}
    virtual ~EventSink() = default;
    virtual void emit(const std::string& contract,
                      const std::string& name,
                      const std::string& value) = 0;
};


