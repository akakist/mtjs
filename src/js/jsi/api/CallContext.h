#pragma once
#include "REF.h"
#include "jsiMsg.h"
#include "Rollback.h"
#include "jsiEvent.h"
#include <vector>
#include <functional>
#include "IMsgSource.h"
#include "FeesSnapshot.h"
#include "db_to_save.h"
#include "cellable.h"
#include "storage_api.h"
#include "cells.h"
#include "ContractHost.h"
#include "CallStack.h"
struct ContractContext;
struct ContractHost;

// ============================================================
// Три режима исполнения контракта:
//   Tx      (tx)      — мутации, msg, газ, roll
//   TxView  (view)    — только чтение, msg родителя
//   Query   (query)   — только чтение, now + esi на входе
// Общее — ExecContext (mode, now, contract, call_stack, host, msg_source, caller, root).
// now — ЕДИНОЕ время: tx/view = block_timestamp, query = wall-clock (сэмпл один раз).
// ============================================================
struct ExecContext : public Refcountable
{
    enum class Mode { Tx, TxView, Query };
    const Mode mode;
    const TIME_mks now;                  // ЕДИНОЕ время, ставит конструктор

    ContractContext* contract = nullptr;
    CallStack* call_stack = nullptr;
    ContractHost* host = nullptr;
    IMsgSource* msg_source = nullptr;    // view наследует родительский; query — nullptr
    CONTRACT_id caller;
    Cellable* root = nullptr;
    std::map<std::string, REF_getter<JSI_Granule>> JSI_Granule_children;

    ExecContext(Mode m, TIME_mks _now) : Refcountable("ExecContext"), mode(m), now(_now) {}
    int depth() const { return (int)call_stack->call_stack.size() - 1; }
};

/// method в транзакции: можно писать
struct TxContext : ExecContext
{
    Rollback* roll = nullptr;
    std::vector<jsiEvent> emits;
    int64_t* value_ptr = nullptr;        // текущий value (нода: &t.value; test_runner: &test_value)
    FeesSnapshot* fees = nullptr;
    std::set<REF_getter<data_base>>* deleted_granules = nullptr;

    TxContext(TIME_mks _now) : ExecContext(Mode::Tx, _now) {}

    void flush(Cellable* root, const REF_getter<JSI_Granule>& g)
    {
        for (auto& z : JSI_Granule_children) z.second->flush(roll, now);
        auto max_time = 0;
        for (auto& z : JSI_Granule_children)
            if (z.second->__update_time > max_time) max_time = z.second->__update_time;
        host->granules->update_contract_time(root, contract->name, TIME_mks{max_time}, roll);
    }
};

/// view, вызванный из tx: только чтение, msg и время — от родителя
struct TxViewContext : ExecContext
{
    TxViewContext(TIME_mks _now) : ExecContext(Mode::TxView, _now) {}
};

/// query из reader: только чтение, now (wall-clock) сэмплируется на входе,
/// block_timestamp — время чейна (для query.blockTimestamp).
struct QueryContext : ExecContext
{
    const TIME_mks block_timestamp;

    QueryContext(TIME_mks _now, TIME_mks _chain)
        : ExecContext(Mode::Query, _now), block_timestamp(_chain) {}
};

/// временный алиас: существующий код продолжает работать, пока переводим пути
using CallContext = TxContext;
