#pragma once
#include "REF.h"
#include "jsiMsg.h"
#include "Rollback.h"
#include "jsiEvent.h"
#include <vector>
#include "IMsgSource.h"
#include "FeesSnapshot.h"
#include "db_to_save.h"
#include "cellable.h"
#include "storage_api.h"
struct ContractContext;
struct CallStack;
struct ContractHost;

struct CallContext {
    REF_getter<ContractContext> contract;
    bool in_view = false;
    int depth = 0;
    REF_getter<CallStack> call_stack;
    REF_getter<ContractHost> host;
    Rollback * roll = nullptr;
    std::vector<jsiEvent> emits;

    IMsgSource* msg_source = nullptr;
    int64_t* value_ptr = nullptr;   // текущий value (нода: &t.value; test_runner: &test_value)

    CONTRACT_id caller;

    FeesSnapshot *fees = nullptr;
    std::set<REF_getter<data_base>> * deleted_granules = nullptr;
    Cellable* root = nullptr;
    // REF_getter<JSI_Granule> jsi_granule_root; 
    std::map<std::string, REF_getter<JSI_Granule>> JSI_Granule_children;
    void flush(const REF_getter<JSI_Granule>& g)
    {
        for(auto& z: JSI_Granule_children)
        {
            z.second->flush(roll);
        }
        auto max_time=0;
        for(auto& z: JSI_Granule_children)
        {
            if(z.second->__update_time> max_time)
            {
                max_time=z.second->__update_time;
            }
        }
        /////////// TODO - update contract actvty
    }
};

