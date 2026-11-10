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
};


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
void data_base::setDirty(Rollback* roll)
{
    MUTEX_INSPECTOR;

    parent->setDirty__(roll);
}
#include "cells.h"


void deleteGranule(data_base* g, Rollback* roll, _db_to_save &db_dump)
{
    auto cell = g->parent;
    if (!cell) return;
    cell->setDirty__(roll);

    while (cell) {
        auto pred = cell->parent;
        if (!pred) return;

        // 1. Помечаем cell (уровень N) на удаление из БД
        db_dump.del(cell->getDbId());

        // 2. Удаляем ссылку на cell из родителя (уровень N-1)
        auto id = cell->m_id;
        pred->children_hashes_mx.erase(id);
        pred->children_ptrs_mx.erase(id);

        // 3. Если у родителя (pred) остались другие дети — останавливаемся.
        //    Иначе — родитель стал пустым листом, удаляем его на следующей итерации.
        if (pred->children_hashes_mx.size())
            return;

        cell = pred;
    }
}
