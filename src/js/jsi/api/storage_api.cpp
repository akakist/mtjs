#include "storage_api.h"
#include "contract_api.h"
#include "CallContext.h"
#include "ContractContext.h"
#include "ContractHost.h"
#include "GranuleIO.h"
#include "cells.h"
#include "commonError.h"
#include "jsiscope.h"
#include "convert.h"
#include "contract_constants.h"
#include <nlohmann/json.hpp>
#include "NodeGranuleIO.h"

using json = nlohmann::json;

static CallContext* call_ctx(JSIContext* ctx) {
    return (CallContext*)JSI_GetContextOpaque(ctx);
}

// ============================================================
// class ids (два)
// ============================================================
static JSIClassID JSI_granule_view_class_id = 0;
static JSIClassID JSI_granule_writable_class_id = 0;

// ============================================================
// wrapper держит и read-only, и writable (ровно один валиден)
// ============================================================
struct JSI_Granule_wrap {
    REF_getter<JSI_GranuleConst> gc;
    REF_getter<JSI_Granule> gw;
};

static void JSI_granule_finalizer(JSIRuntime* rt, JSIValue val) {
    auto* w = (JSI_Granule_wrap*)JSI_GetOpaque(val, JSI_granule_view_class_id);
    if (!w) w = (JSI_Granule_wrap*)JSI_GetOpaque(val, JSI_granule_writable_class_id);
    delete w;
}

static JSIClassDef JSI_granule_view_class = {
    "JSI_GranuleView", .finalizer = JSI_granule_finalizer,
};
static JSIClassDef JSI_granule_writable_class = {
    "JSI_GranuleWritable", .finalizer = JSI_granule_finalizer,
};

// ============================================================
// helpers
// ============================================================
static JSI_Granule_wrap* granule_wrap(JSIContext* ctx, JSIValueConst this_val) {
    auto* w = (JSI_Granule_wrap*)JSI_GetOpaque2(ctx, this_val, JSI_granule_view_class_id);
    if (!w) w = (JSI_Granule_wrap*)JSI_GetOpaque2(ctx, this_val, JSI_granule_writable_class_id);
    return w;
}

static bool is_writable(JSIContext* ctx, JSIValueConst this_val) {
    return JSI_GetClassID(this_val) == JSI_granule_writable_class_id;
}

// полный путь гранулы (read: поле path; writable: обход родителей)
static std::string granule_path(JSIContext* ctx, JSIValueConst this_val) {
    auto* w = granule_wrap(ctx, this_val);
    if (!w) return "";
    if (w->gw.valid()) return w->gw->path();
    if (w->gc.valid()) return w->gc->path;
    return "";
}

// значение гранулы (read или writable — оба дают get())
static bool granule_get_value(JSIContext* ctx, JSIValueConst this_val, std::string* out) {
    auto* w = granule_wrap(ctx, this_val);
    if (!w) return false;
    if (w->gw.valid()) {
        if (!w->gw->cnode.valid()) return false;
        *out = w->gw->cnode->get(); return true;
    }
    if (w->gc.valid()) {
        if (!w->gc->cnode.valid()) return false;
        *out = w->gc->cnode->get(); return true;
    }
    return false;
}

static bool granule_cnode_valid(JSIContext* ctx, JSIValueConst this_val) {
    auto* w = granule_wrap(ctx, this_val);
    if (!w) return false;
    if (w->gw.valid()) return w->gw->cnode.valid();
    if (w->gc.valid()) return w->gc->cnode.valid();
    return false;
}

// ============================================================
// new (read или writable)
// ============================================================
static JSIValue JSI_granule_new(JSIContext* ctx, const REF_getter<JSI_GranuleConst>& gc, const REF_getter<JSI_Granule>& gw) {
    bool writable = gw.valid();
    JSIClassID cid = writable ? JSI_granule_writable_class_id : JSI_granule_view_class_id;
    JSIValue obj = JSI_NewObjectClass(ctx, cid);
    if (JSI_IsException(obj)) return obj;
    auto* w = new JSI_Granule_wrap; w->gc = gc; w->gw = gw;
    JSI_SetOpaque(obj, w);
    return obj;
}

// ============================================================
// do_get_granule (read): по полному пути, без кэша детей
// ============================================================
static REF_getter<JSI_GranuleConst> do_get_granule(JSIContext* ctx, const std::string& path, std::string* err) {
    auto* call = call_ctx(ctx);
    if (!call) throw CommonError("if (!c)");
    auto* io = dynamic_cast<NodeGranuleIO*>(call->host->granules.get());
    if (!io) throw CommonError("if(!io)");

    auto g = new JSI_GranuleConst(path);
    CONTRACT_DATA_id key;
    key.container = path;
    g->cnode = io->getCNodeConst(call->root, call->contract->name, key);
    if (!g->cnode.valid()) {
        if (err) *err = "granule not found: " + key.container;
        return nullptr;
    }
    return g;
}

// ============================================================
// do_get_or_create_granule (write): get, иначе create с ttl
// ============================================================
static REF_getter<JSI_Granule> do_get_or_create_granule(JSIContext* ctx, JSI_Granule* parent,
                                                        const std::string& name, TTL_sec ttl,
                                                        std::string* err) {
    auto* call = call_ctx(ctx);
    if (!call) throw CommonError("if (!c)");
    if (!call->msg_source) throw CommonError("if (!c->msg_source)");
    auto* io = dynamic_cast<NodeGranuleIO*>(call->host->granules.get());
    if (!io) throw CommonError("if(!io)");

    auto g = new JSI_Granule(name, parent);
    CONTRACT_DATA_id key;
    key.container = g->path();

    auto existing = io->getCNode(call->root, call->contract->name, key, TIME_mks{call->msg_source->get_blockTimestamp()}, call->roll);
    if (existing.valid()) {
        g->cnode = existing;
        g->__ttl = existing->ttl().v;
        return g;
    }
    g->cnode = io->createCNode(call->root, call->contract->name, key,
                               TIME_mks{call->msg_source->get_blockTimestamp()},
                               ttl, call->roll);
    if (!g->cnode.valid()) {
        if (err) *err = "create granule failed: " + key.container;
        return nullptr;
    }
    g->__ttl = ttl.v;
    g->do_update_time(call->msg_source->get_blockTimestamp());
    g->setDirtyD();
    return g;
}

// ============================================================
// jsg_get_granule (ttl? -> write; без ttl -> read)
// ============================================================
static JSIValue jsg_get_granule(JSIContext* ctx, JSIValueConst this_val, int argc, JSIValueConst* argv) {
    JSIScope<10,10> scope(ctx);
    if (argc < 1 || argc > 2 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "get_granule(name, ttl?)");
    auto name = scope.toStdString(argv[0]);
    std::string err;
    std::string base = granule_path(ctx, this_val);

    if (argc == 2) {
        auto* c = require_tx_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        int64_t ttl = 0;
        if (JSI_ToInt64(ctx, &ttl, argv[1]) != 0) return JSI_ThrowTypeError(ctx, "ttl must be int64");
        if (ttl <= 0) return JSI_ThrowTypeError(ctx, "ttl must be > 0");
        if (ttl > MAX_GRANULE_TTL) return JSI_ThrowTypeError(ctx, "ttl > MAX_TTL");
        auto* self = granule_wrap(ctx, this_val);
        if (!self || !self->gw.valid()) return JSI_ThrowTypeError(ctx, "get_granule(ttl): read-only granule");
        auto ch = do_get_or_create_granule(ctx, self->gw.get(), name, TTL_sec{ttl}, &err);
        if (!ch.valid()) return JSI_ThrowTypeError(ctx, "get_granule: %s", err.c_str());
        self->gw->children.insert({ch->name, ch});
        return JSI_granule_new(ctx, nullptr, ch);
    } else {
        auto* c = require_readonly_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        std::string child = base.empty() ? name : base + "/" + name;
        auto ch = do_get_granule(ctx, child, &err);
        if (!ch.valid()) return JSI_ThrowTypeError(ctx, "get_granule: %s", err.c_str());
        return JSI_granule_new(ctx, ch, nullptr);
    }
}

// ============================================================
// jsg_exists
// ============================================================
static JSIValue jsg_exists(JSIContext* ctx, JSIValueConst this_val, int argc, JSIValueConst*) {
    if (argc != 0) return JSI_ThrowTypeError(ctx, "exists()");
    auto* c = require_readonly_ctx(ctx);
    if (!c) return JSI_EXCEPTION;
    return JSI_NewBool(ctx, granule_cnode_valid(ctx, this_val));
}

// ============================================================
// jsg_set (только writable)
// ============================================================
static JSIValue jsg_set(JSIContext* ctx, JSIValueConst this_val, int argc, JSIValueConst* argv) {
    if (argc != 1) return JSI_ThrowTypeError(ctx, "set(value)");
    if (!is_writable(ctx, this_val))
        return JSI_ThrowTypeError(ctx, "set is not allowed (read-only storage)");
    auto* c = require_tx_ctx(ctx);
    if (!c) return JSI_EXCEPTION;
    auto* w = granule_wrap(ctx, this_val);
    if (!w || !w->gw.valid()) return JSI_ThrowTypeError(ctx, "invalid this");

    std::string s = serialize_value(ctx, argv[0]);
    if (!is_serializable(s)) return JSI_ThrowTypeError(ctx, "set: cannot serialize");
    w->gw->cnode->set(s, c->block_timestamp);
    w->gw->do_update_time(c->msg_source->get_blockTimestamp());
    w->gw->setDirtyD();
    return JSI_UNDEFINED;
}

// ============================================================
// jsg_del (только writable) — через deleted_granules
// ============================================================
static JSIValue jsg_del(JSIContext* ctx, JSIValueConst this_val, int argc, JSIValueConst*) {
    if (argc != 0) return JSI_ThrowTypeError(ctx, "del()");
    if (!is_writable(ctx, this_val))
        return JSI_ThrowTypeError(ctx, "del is not allowed (read-only storage)");
    auto* c = require_tx_ctx(ctx, false);
    if (!c) return JSI_EXCEPTION;
    auto* w = granule_wrap(ctx, this_val);
    if (!w || !w->gw.valid()) return JSI_ThrowTypeError(ctx, "invalid this");

    auto* cn = dynamic_cast<CNode_prod*>(w->gw->cnode.get());
    if (!cn) return JSI_ThrowTypeError(ctx, "del: not NodeGranuleIO");
    if (c->deleted_granules) c->deleted_granules->insert(cn->data.get());
    w->gw->_deleted = true;
    return JSI_UNDEFINED;
}

// ============================================================
// global: get_granule(path, ttl?)
// ============================================================
static JSIValue JSI_global_get_granule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    JSIScope<10,10> scope(ctx);
    if (argc < 1 || argc > 2 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "get_granule(path, ttl?)");
    auto path = scope.toStdString(argv[0]);
    std::string err;

    if (argc == 2) {
        auto* c = require_tx_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        int64_t ttl = 0;
        if (JSI_ToInt64(ctx, &ttl, argv[1]) != 0) return JSI_ThrowTypeError(ctx, "ttl must be int64");
        if (ttl <= 0) return JSI_ThrowTypeError(ctx, "ttl must be > 0");
        if (ttl > MAX_GRANULE_TTL) return JSI_ThrowTypeError(ctx, "ttl > MAX_TTL");
        auto ch = do_get_or_create_granule(ctx, nullptr, path, TTL_sec{ttl}, &err);
        if (!ch.valid()) return JSI_ThrowInternalError(ctx, "get_granule: %s", err.c_str());
        c->JSI_Granule_children.insert({path, ch});
        return JSI_granule_new(ctx, nullptr, ch);
    } else {
        auto* c = require_readonly_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        auto ch = do_get_granule(ctx, path, &err);
        if (!ch.valid()) return JSI_ThrowTypeError(ctx, "get_granule: %s", err.c_str());
        return JSI_granule_new(ctx, ch, nullptr);
    }
}

// ============================================================
// global: exists_granule(path)
// ============================================================
static JSIValue JSI_global_exists_granule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    JSIScope<10,10> scope(ctx);
    if (argc != 1 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "exists_granule(path)");
    auto* call = require_readonly_ctx(ctx);
    if (!call) return JSI_EXCEPTION;
    auto path = scope.toStdString(argv[0]);

    auto* io = dynamic_cast<NodeGranuleIO*>(call->host->granules.get());
    if (!io) return JSI_ThrowInternalError(ctx, "exists_granule: no io");
    CONTRACT_DATA_id key;
    key.container = path;
    auto cn = io->getCNodeConst(call->root, call->contract->name, key);
    return JSI_NewBool(ctx, cn.valid());
}

// ============================================================
// JSI_global_get — корневая гранула (parent=nullptr)
// ============================================================
static JSIValue JSI_global_get(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    JSIScope<10,10> scope(ctx);
    if (argc < 1 || argc > 2 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "get(name, ttl?)");
    auto name = scope.toStdString(argv[0]);
    std::string err;

    if (argc == 2) {
        auto* c = require_tx_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        int64_t ttl = 0;
        if (JSI_ToInt64(ctx, &ttl, argv[1]) != 0) return JSI_ThrowTypeError(ctx, "ttl must be int64");
        if (ttl <= 0) return JSI_ThrowTypeError(ctx, "ttl must be > 0");
        if (ttl > MAX_GRANULE_TTL) return JSI_ThrowTypeError(ctx, "ttl > MAX_TTL");
        auto ch = do_get_or_create_granule(ctx, nullptr, name, TTL_sec{ttl}, &err);
        if (!ch.valid()) return JSI_ThrowInternalError(ctx, "get: %s", err.c_str());
        c->JSI_Granule_children.insert({name, ch});
        return JSI_granule_new(ctx, nullptr, ch);
    } else {
        auto* c = require_readonly_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        auto ch = do_get_granule(ctx, name, &err);
        if (!ch.valid()) return JSI_NULL;   // ← null (не throw)
        return JSI_granule_new(ctx, ch, nullptr);
    }
}

// ============================================================
// JSI_Granule::get — dispatcher
//   a.get()          → value (JSON, null если пусто)
//   a.get(name)      → child (null если нет)
//   a.get(name, ttl) → child orCreate
// ============================================================
static JSIValue jsg_get(JSIContext* ctx, JSIValueConst this_val, int argc, JSIValueConst* argv) {
    if (argc == 0) {
        auto* c = require_readonly_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        std::string s;
        if (!granule_get_value(ctx, this_val, &s)) return JSI_NULL;
        if (s.empty()) return JSI_NULL;
        try { json j = json::parse(s); return json_to_jsi(ctx, j); }
        catch (...) { return JSI_NULL; }
    }
    JSIScope<10,10> scope(ctx);
    if (argc > 2 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "get(name, ttl?)");
    auto name = scope.toStdString(argv[0]);
    std::string err;
    std::string base = granule_path(ctx, this_val);

    if (argc == 2) {
        auto* c = require_tx_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        int64_t ttl = 0;
        if (JSI_ToInt64(ctx, &ttl, argv[1]) != 0) return JSI_ThrowTypeError(ctx, "ttl must be int64");
        if (ttl <= 0) return JSI_ThrowTypeError(ctx, "ttl must be > 0");
        if (ttl > MAX_GRANULE_TTL) return JSI_ThrowTypeError(ctx, "ttl > MAX_TTL");
        auto* w = granule_wrap(ctx, this_val);
        if (!w || !w->gw.valid()) return JSI_ThrowTypeError(ctx, "get(ttl): read-only granule");
        auto ch = do_get_or_create_granule(ctx, w->gw.get(), name, TTL_sec{ttl}, &err);
        if (!ch.valid()) return JSI_ThrowInternalError(ctx, "get: %s", err.c_str());
        w->gw->children.insert({ch->name, ch});
        return JSI_granule_new(ctx, nullptr, ch);
    } else {
        auto* c = require_readonly_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        std::string child = base.empty() ? name : base + "/" + name;
        auto ch = do_get_granule(ctx, child, &err);
        if (!ch.valid()) return JSI_NULL;   // ← null (не throw)
        return JSI_granule_new(ctx, ch, nullptr);
    }
}

// ============================================================
// прототипы
// ============================================================
static const JSICFunctionListEntry JSI_granule_view_proto[] = {
    JSI_CFUNC_DEF("get", 2, jsg_get),
    JSI_CFUNC_DEF("get_granule", 2, jsg_get_granule),   // alias
    JSI_CFUNC_DEF("exists", 0, jsg_exists),
};

static const JSICFunctionListEntry JSI_granule_writable_proto[] = {
    JSI_CFUNC_DEF("get", 2, jsg_get),
    JSI_CFUNC_DEF("get_granule", 2, jsg_get_granule),
    JSI_CFUNC_DEF("exists", 0, jsg_exists),
    JSI_CFUNC_DEF("set", 1, jsg_set),
    JSI_CFUNC_DEF("del", 0, jsg_del),
};

void register_storage_api(JSIContext* ctx) {
    JSI_NewClassID(&JSI_granule_view_class_id);
    JSI_NewClass(JSI_GetRuntime(ctx), JSI_granule_view_class_id, &JSI_granule_view_class);
    JSI_NewClassID(&JSI_granule_writable_class_id);
    JSI_NewClass(JSI_GetRuntime(ctx), JSI_granule_writable_class_id, &JSI_granule_writable_class);

    {
        JSIValue proto = JSI_NewObject(ctx);
        JSI_SetPropertyFunctionList(ctx, proto, JSI_granule_view_proto,
            sizeof(JSI_granule_view_proto) / sizeof(JSICFunctionListEntry));
        JSI_SetClassProto(ctx, JSI_granule_view_class_id, proto);
    }
    {
        JSIValue proto = JSI_NewObject(ctx);
        JSI_SetPropertyFunctionList(ctx, proto, JSI_granule_writable_proto,
            sizeof(JSI_granule_writable_proto) / sizeof(JSICFunctionListEntry));
        JSI_SetClassProto(ctx, JSI_granule_writable_class_id, proto);
    }

    JSIValue global = JSI_GetGlobalObject(ctx);
    JSI_SetPropertyStr(ctx, global, "get",
        JSI_NewCFunction(ctx, JSI_global_get, "get", 2));
    JSI_SetPropertyStr(ctx, global, "get_granule",
        JSI_NewCFunction(ctx, JSI_global_get_granule, "get_granule", 2));
    JSI_SetPropertyStr(ctx, global, "exists_granule",
        JSI_NewCFunction(ctx, JSI_global_exists_granule, "exists_granule", 1));
    JSI_FreeValue(ctx, global);
}
