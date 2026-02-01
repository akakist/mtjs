#pragma once
#include <quickjs.h>
#include "REF.h"
#include <vector>
struct TimerTask: public Refcountable
{
    JSContext *ctx;
    bool isInterval;
    JSValue cb;
    double timeout;
    bool unrefed=false;
    uint64_t timerId;
    std::vector<JSValue> fargs;
    ~TimerTask()
    {
        JS_FreeValue(ctx,cb);
        for(auto &z: fargs)
        {
            JS_FreeValue(ctx,z);
        }
    }

};

