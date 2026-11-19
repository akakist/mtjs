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
#include "cells.h"
#include "ContractHost.h"
struct ContractContext;
struct CallStack;
struct ContractHost;

// ============================================================
// Три режима исполнения контракта:
//   method  (tx)      — мутации, msg, газ, roll     → TxContext
//   view    (tx)      — только чтение, msg родителя  → TxViewContext
//   query   (reader)  — только чтение, now на входе  → QueryContext
// Общее (root, contract, host, call_stack, msg, block_timestamp) — в ExecContext.
// ============================================================
struct ExecContext {
    enum class Mode { Tx, TxView, Query };
    Mode mode = Mode::Tx;               // конкретный режим проставляет конструктор места
    ContractContext* contract = nullptr;
    int depth = 0;
    CallStack* call_stack = nullptr;
    ContractHost* host = nullptr;
    IMsgSource* msg_source = nullptr;   // view наследует родительский; query — nullptr
    CONTRACT_id caller;
    TIME_mks block_timestamp;           // время чейна для method/view
    Cellable* root = nullptr;
    std::map<std::string, REF_getter<JSI_Granule>> JSI_Granule_children;  // живёт и во вью
};

/// method в транзакции: можно писать
struct TxContext : ExecContext {
    Rollback* roll = nullptr;
    std::vector<jsiEvent> emits;
    int64_t* value_ptr = nullptr;   // текущий value (нода: &t.value; test_runner: &test_value)
    FeesSnapshot* fees = nullptr;
    std::set<REF_getter<data_base>>* deleted_granules = nullptr;

    void flush(Cellable* root, const REF_getter<JSI_Granule>& g)
    {
        for (auto& z : JSI_Granule_children) z.second->flush(roll, block_timestamp);
        auto max_time = 0;
        for (auto& z : JSI_Granule_children)
            if (z.second->__update_time > max_time) max_time = z.second->__update_time;
        host->granules->update_contract_time(root, contract->name, TIME_mks{max_time}, roll);
    }
};

/// view, вызванный из tx: только чтение, msg и время — от родителя
struct TxViewContext : ExecContext {};

/// query из reader: только чтение, now сэмплируется на входе
struct QueryContext : ExecContext {
    TIME_mks now;
};

/// временный алиас: существующий代码 продолжает работать, пока переводим пути
using CallContext = TxContext;


