// contract_api.h
#pragma once
#include "quickjs_i.h"
#include "jsiMsg.h"
#include "CallContext.h"


// базовая: context обязателен
ExecContext* require_ctx__1(JSIContext* ctx);
// read-only: context обязателен, режим любой
ExecContext* require_readonly_ctx(JSIContext* ctx);
// строго method в транзакции: для write-API
TxContext* require_tx_ctx(JSIContext* ctx, bool need_msg_source = true);
// write: только Mode::Tx (+ msg_source, если need_msg_source)
CallContext* require_writable_ctx(JSIContext* ctx, bool need_msg_source = true);

void register_api(JSIContext* ctx);
void delete_prop_str(JSIContext* ctx, JSIValueConst obj, const char* name);
JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& m);
int interrupt_handler(JSIRuntime* rt, void* opaque);