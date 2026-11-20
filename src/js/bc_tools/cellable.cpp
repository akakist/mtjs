#include "cellable.h"
#include "commonError.h"
#include "blake2bHasher.h"
#include <sstream>
#include "bc_address_state.h"
#include "bc_contract.h"
#include "bc_granule.h"
#include "bc_node.h"
#include "bc_node.h"
#include "bc_nodelist.h"
#include "bc_values.h"
#include "bc_vote.h"
#include "bc_contract_balance.h"
#include "bc_state.h"
#include "bc_activity.h"

std::vector<data_base *(*)(Cellable *)> db_constructors = {
    +[](Cellable *p) -> data_base *
    { return new bc_contract(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_address_state(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_node(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_nodelist(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_values(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_granule(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_votes(p); },
    
    +[](Cellable *p) -> data_base * { return new bc_contract_balance(p); }, 

    +[](Cellable *p) -> data_base * { return new bc_state(p); }, 
    +[](Cellable *p) -> data_base * { return new bc_activity(p); }, 

    // bc_deleted: тумбстоун, данных нет. unpack_mx не вызывает конструктор,
    // потому что у такой ячейки valid=false. ВАЖНО: порядок обязан совпадать
    // с hsh::hashers, поэтому заглушка строго последняя.
    +[](Cellable *p) -> data_base * { (void)p; return nullptr; },
};

// Защита от сдвига таблицы: число конструкторов обязано совпадать с HSH_END
// (и порядок — с enum, это проверить нельзя, но размер ловит половину ошибок).
namespace {
struct DbCtorsCheck {
    DbCtorsCheck() {
        if (db_constructors.size() != (size_t)hsh::HSH_END)
            throw CommonError("db_constructors size %zu != HSH_END %d",
                              db_constructors.size(), (int)hsh::HSH_END);
    }
} db_ctors_check;
}


std::string Cellable::getDbId() const
{
    if (parent)
        return parent->getDbId()  + m_id;

    return parent ? m_id : "";
}
std::string data_base::getDbId() const
{
    return parent->getDbId();
}


void Cellable::get_dirty_leaves(_db_to_save &db_dump)
{
    if (!is_dirty)
        return;
    bool has_children=false;
    has_children=children_ptrs_mx.size()>0;

    for (auto &zz : children_ptrs_mx)
    {
        MUTEX_INSPECTOR;
        auto cid = zz.first;
        auto c = zz.second;
        if (c->is_dirty)
        {
            MUTEX_INSPECTOR;
            c->get_dirty_leaves(db_dump);
        }
    }
    if(!has_children)
    {
        auto child_buf = getBuffer_mx();
        db_dump.add(getDbId(), child_buf);


    }

}

void Cellable::calc_tree_hash(_db_to_save &db_dump)
{
    MUTEX_INSPECTOR;
    if (!is_dirty)
        return;

    for (auto &zz : children_ptrs_mx)
    {
        MUTEX_INSPECTOR;
        auto cid = zz.first;
        auto c = zz.second;
        if (c->is_dirty)
        {
            MUTEX_INSPECTOR;
            c->calc_tree_hash(db_dump);
            auto child_buf = c->getBuffer_mx();
            auto ch = blake2b_hash(child_buf);
            if (ch != children_hashes_mx[cid])
            {
                MUTEX_INSPECTOR;
                db_dump.add(c->getDbId(), child_buf);
                children_hashes_mx[cid] = ch;
                {
                    //DEBUG KALL
                    if (c->payload_ctor_idx == hsh::bc_contract_balance) {
                        inBuffer in(child_buf);
                        Cellable chk(nullptr, c->getDbId());
                        chk.unpack_mx(in);
                        if (chk.data.valid()) {
                            auto* b = dynamic_cast<bc_contract_balance*>(chk.data.get());
                            DBG(logErr2("calc_tree_hash: contract_balance dbid=%s amount=%lld",
                                    base16::encode(c->getDbId()).substr(0,16).c_str(), (long long)(b ? b->getAmount() : -1)));
                        }
                    }                    
                }
            }
        }
    }
    is_dirty = false;
}
void data_base::setDirty(Rollback* roll, TIME_mks block_timestamp)
{
    MUTEX_INSPECTOR;
    if (getenv("MTJS_TREE_DUMP") && parent)
        logErr2("[TRACE setDirty] id=%s ts=%lld roll=%d h_before=%s",
                base16::encode(parent->getDbId()).substr(0,16).c_str(),
                (long long)block_timestamp.v, (int)(roll != nullptr),
                base16::encode(blake2b_hash(parent->getBuffer_mx()).container).substr(0,16).c_str());
    parent->setDirty__(roll,block_timestamp);
}
#include "cells.h"


void deleteGranule(data_base* g, Rollback* roll, _db_to_save &db_dump, TIME_mks block_timestamp)
{
    // Тумбстоун без отдельных полей:
    //  - тип ячейки становится bc_deleted (данных нет, data = nullptr);
    //  - время удаления хранится в last_block_timestamp (его выставляет setDirty__);
    //  - структура дерева не меняется, поэтому удаление выразимо в дельта-синке;
    //  - физически такая ячейка вычищается компакцией по last_block_timestamp.
    auto cell = g ? g->parent : nullptr;
    if (!cell) return;
    if (cell->payload_ctor_idx == hsh::bc_deleted) return;   // идемпотентно

    cell->setDirty__(roll, block_timestamp);   // в roll сохранится старый буфер
    cell->payload_ctor_idx = hsh::bc_deleted;
    cell->data = nullptr;
    db_dump.add(cell->getDbId(), cell->getBuffer_mx());
}
