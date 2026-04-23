#pragma once
#include "jHolder.h"
#include "rcf.h"
#include "broadcaster.h"
#include "async_task.h"
#include <optional>
#include <quickjs.h>
#include "THASH_id.h"

struct JSPromise
{
    JSContext *ctx;
    // JSValue promise_data[2];
    JSValue resolve;
    JSValue reject;
    ~JSPromise()
    {
        JS_FreeValue(ctx, resolve);
        JS_FreeValue(ctx, reject);
    }
};
struct mtjs_opaque
{
    ListenerBase* listener_;
    Broadcaster * broadcaster;
    bool rpcBlockExit=false;

    REF_getter<RCF> rcf=nullptr;
    REF_getter< op_deque> async_deque=new op_deque;

    std::map<std::string, JHolder> rpc_on_srv_callbacks;
    std::map<std::string, JHolder> rpc_on_cli_callbacks;
    std::optional<JHolder> telnet_callback;
    std::map<THASH_id, JSPromise > node_req_promises;
    std::optional<JHolder> tx_subscription_cb;

    void clear()
    {
        rcf=nullptr;
        async_deque=nullptr;

        rpc_on_srv_callbacks.clear();
        rpc_on_cli_callbacks.clear();
        telnet_callback.reset();

    }
};
