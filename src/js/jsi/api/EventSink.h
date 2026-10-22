#pragma once
#include "REF.h"
#include <string>
#include "CONTRACT_id.h"
struct EventSink : public Refcountable {
    EventSink() : Refcountable("EventSink") {}
    virtual ~EventSink() = default;
    virtual void emit(const CONTRACT_id& contract,
                      const std::string& name,
                      const std::string& value) = 0;
};


