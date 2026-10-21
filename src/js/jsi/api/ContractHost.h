#pragma once
#include "REF.h"
#include "GranuleIO.h"
#include "EventSink.h"
#include "Clock.h"
#include "ContractSourceIO.h"

struct ContractHost : public Refcountable {
    REF_getter<GranuleIO> granules;
    REF_getter<EventSink> events;
    REF_getter<Clock> clock;
    REF_getter<ContractSourceIO> source;

    std::map<std::string, REF_getter<ContractContext>> contract_cache;

    ContractHost() : Refcountable("ContractHost") {}
    virtual ~ContractHost() = default;

    // ---- загрузка/получение контрактов (методы) ----
    REF_getter<ContractContext> load_contract(const std::string& name);
    REF_getter<ContractContext> get_contract(const std::string& name);

    // ---- фабрики ----
};

