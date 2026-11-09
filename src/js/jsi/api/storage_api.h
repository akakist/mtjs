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
// JSI_Granule — базовый (read-only): get_granule, exists, get
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
        while (p) { p->update_time(t + ttl); p = p->parent; }
    }
    void setDirtyD() { dirtyD = true; }

    void flush(Rollback* roll) {
        if (_deleted) return;   // удаление — через deleted_granules (jsg_del)
        if (__update_time) cnode->update_time(__update_time, roll);
        if (dirtyD) cnode->setDirty(roll);
        for (auto& z : children) z.second->flush(roll);
    }
};

// ============================================================
// JSI_GranuleWritable — write (method): + get_granule(name,ttl), set, del
// ============================================================
struct JSI_GranuleWritable : public JSI_Granule {
    JSI_GranuleWritable(const std::string& nm, JSI_Granule* par)
        : JSI_Granule(nm, par) {}
};

void register_storage_api(JSIContext* ctx);