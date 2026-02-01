#pragma once
#include "jHolder.h"
#include "rcf.h"
#include "broadcaster.h"
#include "async_task.h"
#include <optional>

struct mtjs_opaque
{
    ListenerBase* listener;
    Broadcaster * broadcaster;
    bool rpcBlockExit=false;

    REF_getter<RCF> rcf=nullptr;
    REF_getter< op_deque> async_deque=new op_deque;

    std::map<std::string, JHolder> rpc_on_srv_callbacks;
    std::map<std::string, JHolder> rpc_on_cli_callbacks;
    std::optional<JHolder> telnet_callback;

    void clear()
    {
        rcf=nullptr;
        async_deque=nullptr;

        rpc_on_srv_callbacks.clear();
        rpc_on_cli_callbacks.clear();
        telnet_callback.reset();

    }
};
