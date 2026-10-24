#include "mtjs_block.h"
#include "bcEvent.h"
#include "md/md_BlockAccepted2REQ.h"
#include "md/md_attachment_data.h"
#include "THASH_id.h"
#include "base16.h"
#include "common/jsscope.h"
#include "common/js_tools.h"
#include "js_tools.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

JSClassID js_block_class_id = 0;

struct JS_Block {
    REF_getter<MsgData::BlockAccepted2REQ> ba2;
    REF_getter<MsgData::attachment_data>   att;
    std::vector<THASH_id>                  tx_hashes;
    bool                                   finalized = false;
    ~JS_Block() {}
};

// ---- emit_node_to_js (перенесён из mtjsService.cpp) ----
#ifdef KALL
static void to_json_emit_node(nlohmann::json& j, const EmitNode& node)
{
    MUTEX_INSPECTOR;
    XTRY;
    nlohmann::json emits_array = nlohmann::json::array();
    for (const auto& emit_pair : node.emits)
    {
        XTRY;
        MUTEX_INSPECTOR;
        nlohmann::json emit_obj;
        emit_obj["cmd"] = emit_pair.first;
        try {
            MUTEX_INSPECTOR;
            emit_obj["data"] = nlohmann::json::parse(emit_pair.second);
        } catch (std::exception& e) {
            logErr2("Failed to parse JSON for emit data: %s, error: %s", emit_pair.second.c_str(), e.what());
        }
        emits_array.push_back(emit_obj);
        XPASS;
    }
    j["emits"] = emits_array;
    for (const auto& child_pair : node.children) {
        XTRY;
        j["children"][child_pair.first] = child_pair.second;
        XPASS;
    }
    XPASS;
}
#endif
JSValue emit_node_to_js(JSContext* ctx, const EmitNode& node)
{
    JSValue obj = JS_NewObject(ctx);

    JSValue emits_arr = JS_NewArray(ctx);
    for (size_t i = 0; i < node.emits.size(); ++i) {
        JSValue emit_arr = JS_NewArray(ctx);
        JS_SetPropertyUint32(ctx, emit_arr, 0, JS_NewString(ctx, node.emits[i].first.c_str()));
        JSValue data_obj = JS_ParseJSON(ctx, node.emits[i].second.c_str(), node.emits[i].second.size(), "");
        JS_SetPropertyUint32(ctx, emit_arr, 1, data_obj);
        JS_SetPropertyUint32(ctx, emits_arr, i, emit_arr);
    }
    JS_SetPropertyStr(ctx, obj, "emits", emits_arr);

    if (!node.children.empty()) {
        JSValue children_obj = JS_NewObject(ctx);
        for (const auto& [key, child] : node.children) {
            JS_SetPropertyStr(ctx, children_obj, key.c_str(), emit_node_to_js(ctx, child));
        }
        JS_SetPropertyStr(ctx, obj, "children", children_obj);
    }

    return obj;
}

// ---- аксессоры ----

static JSValue js_block_get_tx_hashes(JSContext* ctx, JSValueConst this_val)
{
    JS_Block* b = static_cast<JS_Block*>(JS_GetOpaque2(ctx, this_val, js_block_class_id));
    if (!b) return JS_EXCEPTION;
    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < b->tx_hashes.size(); ++i) {
        auto hex = base16::encode(b->tx_hashes[i].container);
        JS_SetPropertyUint32(ctx, arr, (uint32_t)i, JS_NewStringLen(ctx, hex.data(), hex.size()));
    }
    return arr;
}

static JSValue js_block_get_finalized(JSContext* ctx, JSValueConst this_val)
{
    JS_Block* b = static_cast<JS_Block*>(JS_GetOpaque2(ctx, this_val, js_block_class_id));
    if (!b) return JS_EXCEPTION;
    return JS_NewBool(ctx, b->finalized);
}

static JSValue js_block_get_block_hash(JSContext* ctx, JSValueConst this_val)
{
    JS_Block* b = static_cast<JS_Block*>(JS_GetOpaque2(ctx, this_val, js_block_class_id));
    if (!b) return JS_EXCEPTION;
    if (!b->ba2.valid() || !b->ba2->blockAcceptedREQ.valid())
        return JS_NewString(ctx, "");
    auto hex = base16::encode(b->ba2->blockAcceptedREQ->blockInfo->new_root_hash1.container);
    return JS_NewStringLen(ctx, hex.data(), hex.size());
}

static JSValue js_block_get_block_timestamp(JSContext* ctx, JSValueConst this_val)
{
    JS_Block* b = static_cast<JS_Block*>(JS_GetOpaque2(ctx, this_val, js_block_class_id));
    if (!b) return JS_EXCEPTION;
    if (!b->ba2.valid() || !b->ba2->blockAcceptedREQ.valid())
        return JS_NewInt64(ctx, 0);
    return JS_NewInt64(ctx, (int64_t)b->ba2->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp);
}

static const EmitNode* find_tx_node(JS_Block* b, const std::string& tx_hex)
{
    if (!b->att.valid()) return nullptr;
    auto it = b->att->blockRoot.children.find(tx_hex);
    if (it == b->att->blockRoot.children.end()) return nullptr;
    return &it->second;
}

static JSValue js_block_status(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSScope<10,10> scope(ctx);
    JS_Block* b = static_cast<JS_Block*>(JS_GetOpaque2(ctx, this_val, js_block_class_id));
    if (!b) return JS_EXCEPTION;
    if (argc != 1 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "status(tx_hash)");
    auto tx_hex = scope.toStdString(argv[0]);
    const EmitNode* n = find_tx_node(b, tx_hex);
    if (!n || n->emits.empty()) return JS_NewString(ctx, "unknown");
    const std::string& cmd = n->emits[0].first;
    if (cmd == "error") return JS_NewString(ctx, "err");
    if (cmd == "result") {
        try {
            auto j = nlohmann::json::parse(n->emits[0].second);
            if (j.contains("success") && j["success"].is_boolean())
                return JS_NewString(ctx, j["success"].get<bool>() ? "ok" : "err");
        } catch (...) {}
    }
    return JS_NewString(ctx, "unknown");
}
static JSValue js_block_errstr(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSScope<10,10> scope(ctx);
    JS_Block* b = static_cast<JS_Block*>(JS_GetOpaque2(ctx, this_val, js_block_class_id));
    if (!b) return JS_EXCEPTION;
    if (argc != 1 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "errstr(tx_hash)");
    auto tx_hex = scope.toStdString(argv[0]);
    const EmitNode* n = find_tx_node(b, tx_hex);
    if (!n || n->emits.empty()) return JS_NULL;
    const std::string& cmd = n->emits[0].first;
    if (cmd != "error" && cmd != "result") return JS_NULL;
    try {
        auto j = nlohmann::json::parse(n->emits[0].second);
        bool is_err = (cmd == "error");
        if (cmd == "result" && j.contains("success") && j["success"].is_boolean() && !j["success"].get<bool>())
            is_err = true;
        if (is_err && j.contains("error") && j["error"].is_string()) {
            auto s = j["error"].get<std::string>();
            return JS_NewStringLen(ctx, s.data(), s.size());
        }
    } catch (...) {}
    return JS_NULL;
}
static JSValue js_block_emits(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSScope<10,10> scope(ctx);
    JS_Block* b = static_cast<JS_Block*>(JS_GetOpaque2(ctx, this_val, js_block_class_id));
    if (!b) return JS_EXCEPTION;
    if (argc != 1 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "emits(tx_hash)");
    auto tx_hex = scope.toStdString(argv[0]);
    const EmitNode* n = find_tx_node(b, tx_hex);
    if (!n) return JS_NULL;
    JSValue node_js = emit_node_to_js(ctx, *n);
    JSValue emits = JS_GetPropertyStr(ctx, node_js, "emits");
    JS_FreeValue(ctx, node_js);
    return emits;
}

static JSValue js_block_children(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSScope<10,10> scope(ctx);
    JS_Block* b = static_cast<JS_Block*>(JS_GetOpaque2(ctx, this_val, js_block_class_id));
    if (!b) return JS_EXCEPTION;
    if (argc != 1 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "children(tx_hash)");
    auto tx_hex = scope.toStdString(argv[0]);
    const EmitNode* n = find_tx_node(b, tx_hex);
    if (!n) return JS_NULL;
    JSValue node_js = emit_node_to_js(ctx, *n);
    JSValue children = JS_GetPropertyStr(ctx, node_js, "children");
    JS_FreeValue(ctx, node_js);
    return children;
}

static const JSCFunctionListEntry js_block_proto_funcs[] = {
    JS_CGETSET_DEF("tx_hashes",       js_block_get_tx_hashes,       NULL),
    JS_CGETSET_DEF("finalized",       js_block_get_finalized,       NULL),
    JS_CGETSET_DEF("block_hash",      js_block_get_block_hash,      NULL),
    JS_CGETSET_DEF("block_timestamp", js_block_get_block_timestamp, NULL),
    JS_CFUNC_DEF("status",   1, js_block_status),
    JS_CFUNC_DEF("errstr",   1, js_block_errstr),
    JS_CFUNC_DEF("emits",    1, js_block_emits),
    JS_CFUNC_DEF("children", 1, js_block_children),
};

static void js_block_finalizer(JSRuntime* rt, JSValue val)
{
    JS_Block* b = static_cast<JS_Block*>(JS_GetOpaque(val, js_block_class_id));
    if (b) delete b;
}

static JSClassDef js_block_class = {
    "Block",
    .finalizer = js_block_finalizer,
};

void register_block_class(JSContext* ctx)
{
    JS_NewClassID(&js_block_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_block_class_id, &js_block_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_block_proto_funcs,
                               sizeof(js_block_proto_funcs) / sizeof(JSCFunctionListEntry));
    JS_SetClassProto(ctx, js_block_class_id, proto);
}

JSValue js_block_new(JSContext* ctx,
                     const REF_getter<MsgData::BlockAccepted2REQ>& ba2,
                     const REF_getter<MsgData::attachment_data>& att,
                     const std::vector<THASH_id>& tx_hashes,
                     bool finalized)
{
    JS_Block* b = new JS_Block;
    b->ba2 = ba2;
    b->att = att;
    b->tx_hashes = tx_hashes;
    b->finalized = finalized;
    {
        std::string txh;
        for (auto& h : tx_hashes) { txh += base16::encode(h.container) + " "; }
        std::string ch;
        if (att.valid()) for (auto& kv : att->blockRoot.children) { ch += kv.first + "(" + std::to_string(kv.second.emits.size()) + ") "; }

       for (auto& kv : att->blockRoot.children) {
            std::string chs;
            for (auto& c : kv.second.children) {
                chs += c.first + "(";
                for (auto& e : c.second.emits) chs += e.first + ",";
                chs += ") ";
            }
            ch += kv.first + "(emits=" + std::to_string(kv.second.emits.size()) + " children=[" + chs + "]) ";
        } 
        logErr2("js_block_new: ba2=%d att=%d blockRoot_children=%d tx_hashes=%d | tx=[%s] children=[%s]",
            ba2.valid(), att.valid(),
            att.valid() ? (int)att->blockRoot.children.size() : -1,
            (int)tx_hashes.size(),
            txh.c_str(), ch.c_str());
    }

    JSValue obj = JS_NewObjectClass(ctx, js_block_class_id);
    if (JS_IsException(obj)) {
        delete b;
        return obj;
    }
    JS_SetOpaque(obj, b);
    return obj;
}