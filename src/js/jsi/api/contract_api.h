// contract_api.h
#pragma once
#include "quickjs_i.h"
#include "jsiMsg.h"
#include "CallContext.h"

struct CallContext;

// базовая: context обязателен
CallContext* require_ctx__1(JSIContext* ctx);
// read-only: context обязателен, in_view — ок
CallContext* require_readonly_ctx(JSIContext* ctx);
// write: context + !in_view + (msg_source, если need_msg_source)
CallContext* require_writable_ctx(JSIContext* ctx, bool need_msg_source = true);

void register_api(JSIContext* ctx);
void delete_prop_str(JSIContext* ctx, JSIValueConst obj, const char* name);
JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& m);
int interrupt_handler(JSIRuntime* rt, void* opaque);