#pragma once
#include "REF.h"
#include "GranuleIO.h"
#include "EventSink.h"
#include "Clock.h"
#include "ContractSourceIO.h"
#include "ContractContext.h"

struct ContractHost : public Refcountable {
    REF_getter<GranuleIO> granules;
    REF_getter<ContractSourceIO> source;

    std::map<CONTRACT_id, REF_getter<ContractContext>> contract_cache;

    ContractHost() : Refcountable("ContractHost") {}
    virtual ~ContractHost() = default;

    // ---- загрузка/получение контрактов (методы) ----
    REF_getter<ContractContext> load_contract(const CONTRACT_id& name);
    REF_getter<ContractContext> get_contract(const CONTRACT_id& name);

    // ---- фабрики ----
};

