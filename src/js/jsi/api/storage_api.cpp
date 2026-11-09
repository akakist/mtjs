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
// wrappers
// ============================================================
struct JSI_Granule_wrap {
    REF_getter<JSI_Granule> g;
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
static REF_getter<JSI_Granule> JSI_granule_get(JSIContext* ctx, JSIValueConst this_val) {
    auto* w = (JSI_Granule_wrap*)JSI_GetOpaque2(ctx, this_val, JSI_granule_view_class_id);
    if (!w) w = (JSI_Granule_wrap*)JSI_GetOpaque2(ctx, this_val, JSI_granule_writable_class_id);
    if (!w) return nullptr;
    return w->g;
}

static bool is_writable(JSIContext* ctx, JSIValueConst this_val) {
    return JSI_GetClassID(this_val) == JSI_granule_writable_class_id;
}

// ============================================================
// new (writable flag)
// ============================================================
static JSIValue JSI_granule_new(JSIContext* ctx, const REF_getter<JSI_Granule>& g, bool writable) {
    JSIClassID cid = writable ? JSI_granule_writable_class_id : JSI_granule_view_class_id;
    JSIValue obj = JSI_NewObjectClass(ctx, cid);
    if (JSI_IsException(obj)) return obj;
    auto* w = new JSI_Granule_wrap; w->g = g;
    JSI_SetOpaque(obj, w);
    return obj;
}

// ============================================================
// do_get_granule (read: throw, если нет)
// ============================================================
static REF_getter<JSI_Granule> do_get_granule(JSIContext* ctx, JSI_Granule* parent,
                                              const std::string& name, std::string* err)
{
    auto* c = call_ctx(ctx);
    if (!c) throw CommonError("if (!c)");
    auto* io = dynamic_cast<NodeGranuleIO*>(c->host->granules.get());
    if (!io) throw CommonError("if(!io)");

    auto g = new JSI_Granule(name, parent);
    CONTRACT_DATA_id key;
    key.container = g->path();

    g->cnode = io->getCNode(c->root, c->contract->name, key);
    if (!g->cnode.valid()) {
        if (err) *err = "granule not found: " + key.container;
        return nullptr;
    }
    g->__ttl = g->cnode->ttl();
    return g;
}

// ============================================================
// do_get_or_create_granule (write: get, иначе create с ttl)
// ============================================================
static REF_getter<JSI_Granule> do_get_or_create_granule(JSIContext* ctx, JSI_Granule* parent,
                                                        const std::string& name, int64_t ttl,
                                                        std::string* err)
{
    auto* c = call_ctx(ctx);
    if (!c) throw CommonError("if (!c)");
    if (!c->msg_source) throw CommonError("if (!c->msg_source)");
    auto* io = dynamic_cast<NodeGranuleIO*>(c->host->granules.get());
    if (!io) throw CommonError("if(!io)");

    auto g = new JSI_GranuleWritable(name, parent);
    CONTRACT_DATA_id key;
    key.container = g->path();

    // 1) get
    auto existing = io->getCNode(c->root, c->contract->name, key);
    if (existing.valid()) {
        g->cnode = existing;
        g->__ttl = existing->ttl();
        return g;
    }
    // 2) create
    g->cnode = io->createCNode(c->root, c->contract->name, key,
                               (time_t)c->msg_source->get_blockTimestamp(),
                               (int)ttl, c->roll);
    if (!g->cnode.valid()) {
        if (err) *err = "create granule failed: " + key.container;
        return nullptr;
    }
    g->__ttl = ttl;
    g->do_update_time(c->msg_source->get_blockTimestamp());
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

    auto self = JSI_granule_get(ctx, this_val);
    if (!self.valid()) return JSI_ThrowTypeError(ctx, "invalid this");
    auto name = scope.toStdString(argv[0]);
    std::string err;

    if (argc == 2) {
        auto* c = require_writable_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        int64_t ttl = 0;
        if (JSI_ToInt64(ctx, &ttl, argv[1]) != 0) return JSI_ThrowTypeError(ctx, "ttl must be int64");
        if (ttl <= 0) return JSI_ThrowTypeError(ctx, "ttl must be > 0");
        if (ttl > MAX_GRANULE_TTL) return JSI_ThrowTypeError(ctx, "ttl > MAX_TTL");
        auto ch = do_get_or_create_granule(ctx, self.get(), name, ttl, &err);
        if (!ch.valid()) return JSI_ThrowTypeError(ctx, "get_granule: %s", err.c_str());
        self->children.insert({ch->name, ch});
        return JSI_granule_new(ctx, ch, /*writable=*/true);
    } else {
        auto* c = require_readonly_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        auto ch = do_get_granule(ctx, self.get(), name, &err);
        if (!ch.valid()) return JSI_ThrowTypeError(ctx, "get_granule: %s", err.c_str());
        self->children.insert({ch->name, ch});
        return JSI_granule_new(ctx, ch, /*writable=*/false);
    }
}

// ============================================================
// jsg_exists
// ============================================================
static JSIValue jsg_exists(JSIContext* ctx, JSIValueConst this_val, int argc, JSIValueConst*) {
    if (argc != 0) return JSI_ThrowTypeError(ctx, "exists()");
    auto* c = require_readonly_ctx(ctx);
    if (!c) return JSI_EXCEPTION;
    auto self = JSI_granule_get(ctx, this_val);
    if (!self.valid()) return JSI_ThrowTypeError(ctx, "invalid this");
    return JSI_NewBool(ctx, self->cnode.valid());
}

// ============================================================
// jsg_get
// ============================================================
static JSIValue jsg_get(JSIContext* ctx, JSIValueConst this_val, int argc, JSIValueConst*) {
    if (argc != 0) return JSI_ThrowTypeError(ctx, "get()");
    auto* c = require_readonly_ctx(ctx);
    if (!c) return JSI_EXCEPTION;
    auto self = JSI_granule_get(ctx, this_val);
    if (!self.valid()) return JSI_ThrowTypeError(ctx, "invalid this");
    if (!self->cnode.valid()) return JSI_NULL;
    auto s = self->cnode->get();
    try { json j = json::parse(s); return json_to_jsi(ctx, j); }
    catch (...) { return JSI_NULL; }
}

// ============================================================
// jsg_set (только writable)
// ============================================================
static JSIValue jsg_set(JSIContext* ctx, JSIValueConst this_val, int argc, JSIValueConst* argv) {
    if (argc != 1) return JSI_ThrowTypeError(ctx, "set(value)");
    if (!is_writable(ctx, this_val))
        return JSI_ThrowTypeError(ctx, "set is not allowed (read-only storage)");
    auto* c = require_writable_ctx(ctx);
    if (!c) return JSI_EXCEPTION;
    auto self = JSI_granule_get(ctx, this_val);
    if (!self.valid()) return JSI_ThrowTypeError(ctx, "invalid this");

    std::string s = serialize_value(ctx, argv[0]);
    if (!is_serializable(s)) return JSI_ThrowTypeError(ctx, "set: cannot serialize");
    self->cnode->set(s);
    self->do_update_time(c->msg_source->get_blockTimestamp());
    self->setDirtyD();
    return JSI_UNDEFINED;
}

// ============================================================
// jsg_del (только writable) — через deleted_granules
// ============================================================
static JSIValue jsg_del(JSIContext* ctx, JSIValueConst this_val, int argc, JSIValueConst*) {
    if (argc != 0) return JSI_ThrowTypeError(ctx, "del()");
    if (!is_writable(ctx, this_val))
        return JSI_ThrowTypeError(ctx, "del is not allowed (read-only storage)");
    auto* c = require_writable_ctx(ctx, /*need_msg_source=*/false);
    if (!c) return JSI_EXCEPTION;
    auto self = JSI_granule_get(ctx, this_val);
    if (!self.valid()) return JSI_ThrowTypeError(ctx, "invalid this");

    auto* cn = dynamic_cast<CNode_prod*>(self->cnode.get());
    if (!cn) return JSI_ThrowTypeError(ctx, "del: not NodeGranuleIO");
    if (c->deleted_granules) c->deleted_granules->insert(cn->data.get());
    self->_deleted = true;
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
        auto* c = require_writable_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        int64_t ttl = 0;
        if (JSI_ToInt64(ctx, &ttl, argv[1]) != 0) return JSI_ThrowTypeError(ctx, "ttl must be int64");
        if (ttl <= 0) return JSI_ThrowTypeError(ctx, "ttl must be > 0");
        if (ttl > MAX_GRANULE_TTL) return JSI_ThrowTypeError(ctx, "ttl > MAX_TTL");
        auto ch = do_get_or_create_granule(ctx, nullptr, path, ttl, &err);
        if (!ch.valid()) return JSI_ThrowInternalError(ctx, "get_granule: %s", err.c_str());
        c->JSI_Granule_children.insert({path, ch});
        return JSI_granule_new(ctx, ch, /*writable=*/true);
    } else {
        auto* c = require_readonly_ctx(ctx);
        if (!c) return JSI_EXCEPTION;
        auto ch = do_get_granule(ctx, nullptr, path, &err);
        if (!ch.valid()) return JSI_ThrowTypeError(ctx, "get_granule: %s", err.c_str());
        c->JSI_Granule_children.insert({path, ch});
        return JSI_granule_new(ctx, ch, /*writable=*/false);
    }
}

// ============================================================
// global: exists_granule(path)
// ============================================================
static JSIValue JSI_global_exists_granule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    JSIScope<10,10> scope(ctx);
    if (argc != 1 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "exists_granule(path)");
    auto* c = require_readonly_ctx(ctx);
    if (!c) return JSI_EXCEPTION;
    auto path = scope.toStdString(argv[0]);

    auto* io = dynamic_cast<NodeGranuleIO*>(c->host->granules.get());
    if (!io) return JSI_ThrowInternalError(ctx, "exists_granule: no io");
    CONTRACT_DATA_id key;
    key.container = path;
    auto cn = io->getCNode(c->root, c->contract->name, key);
    return JSI_NewBool(ctx, cn.valid());
}

// ============================================================
// прототипы
// ============================================================
static const JSICFunctionListEntry JSI_granule_view_proto[] = {
    JSI_CFUNC_DEF("get_granule", 2, jsg_get_granule),
    JSI_CFUNC_DEF("exists",      0, jsg_exists),
    JSI_CFUNC_DEF("get",         0, jsg_get),
};

static const JSICFunctionListEntry JSI_granule_writable_proto[] = {
    JSI_CFUNC_DEF("get_granule", 2, jsg_get_granule),
    JSI_CFUNC_DEF("exists",      0, jsg_exists),
    JSI_CFUNC_DEF("get",         0, jsg_get),
    JSI_CFUNC_DEF("set",         1, jsg_set),
    JSI_CFUNC_DEF("del",         0, jsg_del),
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
    JSI_SetPropertyStr(ctx, global, "get_granule",
        JSI_NewCFunction(ctx, JSI_global_get_granule, "get_granule", 2));
    JSI_SetPropertyStr(ctx, global, "exists_granule",
        JSI_NewCFunction(ctx, JSI_global_exists_granule, "exists_granule", 1));
    JSI_FreeValue(ctx, global);
}