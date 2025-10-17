#pragma once
#include "commonError.h"
#include <quickjs.h>
struct JHolder
{
    JSValue listener;
    JSContext* ctx;
    JHolder(JSContext *_c,JSValue _listener) : ctx(_c), listener(JS_DupValue(_c,_listener)) {
        DBG(iUtils->mem_add_ptr("JHolder",this));

    }
    JHolder(const JHolder& a):listener(a.listener),ctx(a.ctx) {
        JS_DupValue(ctx,a.listener);
        DBG(iUtils->mem_add_ptr("JHolder",this));
    }
    ~JHolder() {
        JS_FreeValue(ctx, listener);
        DBG(iUtils->mem_remove_ptr("JHolder",this));

    }
    JHolder& operator=(const JHolder&) = delete;

};




int operator< (const JHolder &a, const JHolder &b);

inline int operator< (const JHolder &a, const JHolder &b)
{
    if(a.ctx != b.ctx) return a.ctx < b.ctx;
    if(a.listener.tag != b.listener.tag) return a.listener.tag < b.listener.tag;
    if(a.listener.u.ptr != b.listener.u.ptr) return  a.listener.u.ptr < b.listener.u.ptr;
    return false;
}

