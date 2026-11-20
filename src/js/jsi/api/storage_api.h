#pragma once
#include "REF.h"
#include "quickjs_i.h"
#include "CONTRACT_id.h"
#include "Rollback.h"
#include "db_to_save.h"
#include <string>
#include <set>
#include <map>
#include "IDatabase.h"
#include "GranuleIO.h"

// ============================================================
// JSI_GranuleConst — read-only: только get/ttl/exists.
// Нет set/del/flush/update_time — компилятор не даёт писать.
// ============================================================
struct JSI_GranuleConst : public Refcountable {
    std::string path;   // полный путь (read-only гранула эфемерна: без parent/children)
    REF_getter<const CNodeConst> cnode = nullptr;

    JSI_GranuleConst(const std::string& p)
        : Refcountable("JSI_GranuleConst"), path(p) {}
    virtual ~JSI_GranuleConst() = default;
};

// ============================================================
// JSI_Granule — writable (method): set/del/get_granule(ttl), flush
// ============================================================
struct JSI_Granule : public Refcountable {
    JSI_Granule* parent = nullptr;
    std::string name;
    bool _deleted = false;

    time_t __update_time = 0;
    int64_t __ttl = 0;

    std::map<std::string, REF_getter<JSI_Granule>> children;
    REF_getter<CNode> cnode = nullptr;

    bool dirtyA = false;
    bool dirtyD = false;

    JSI_Granule(const std::string& nm, JSI_Granule* par)
        : Refcountable("JSI_Granule"), name(nm), parent(par) {}
    virtual ~JSI_Granule() = default;

    std::string path() const {
        std::string p = name;
        auto* par = parent;
        while (par) { p = par->name + "/" + p; par = par->parent; }
        return p;
    }

    void update_time(time_t t) {
        if (t > __update_time) { __update_time = t; dirtyA = true; }
    }
    void do_update_time(time_t t) {
        update_time(t);
        auto ttl = cnode->ttl();
        auto* p = parent;
        while (p) { p->update_time(t + ttl.v); p = p->parent; }
    }
    void setDirtyD() { dirtyD = true; }

    void flush(Rollback* roll, TIME_mks block_timestamp) {
        if (_deleted) return;
        if (__update_time) cnode->update_time(TIME_mks{__update_time}, roll);
        if (dirtyD) cnode->setDirty(roll, block_timestamp);
        for (auto& z : children) z.second->flush(roll, block_timestamp);
    }
};

using JSI_GranuleWritable = JSI_Granule;   // совместимость

void register_storage_api(JSIContext* ctx);
