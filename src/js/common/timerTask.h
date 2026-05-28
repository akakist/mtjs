#pragma once
#include <quickjs.h>
#include "REF.h"
#include <vector>
#include "jsValueGuard.h"
struct TimerTask: public Refcountable
{

    JSContext *ctx;
    bool isInterval;
    JSValueGuard cb;
    double timeout;
    bool unrefed=false;
    uint64_t timerId;
    std::vector<JSValueGuard> fargs;
    TimerTask():Refcountable("ToimerTask") {}
    ~TimerTask()
    {
        // JS_FreeValue(ctx,cb);
        // for(auto &z: fargs)
        // {
        //     JS_FreeValue(ctx,z);
        // }
    }

};

