#pragma once
#include <quickjs.h>
#include <map>
#include <vector>
#include <string>
#include <set>
#include "jsscope.h"
#include "js_tools.h"
#include "jsValueGuard.h"


struct EventEmitter: public Refcountable
{

    int maxListeners=10;
    std::map<std::string, std::set<JSValueGuard>> m_listeners;
    JSContext* ctx;
    const char *comment;
    EventEmitter(JSContext* _ctx, const char* _comment) : Refcountable("EventEmitter"),
        ctx(_ctx),  comment(_comment)
    {
    }

    int emit(const std::string& event, int argc, JSValue *argv);

    void on(const std::string& event, const JSValueGuard& listener);
    std::vector<JSValueGuard> listeners(const std::string& event);

    ~EventEmitter()
    {
    }

};
JSValue new_event_emitter(JSContext *ctx,const REF_getter<EventEmitter>& emitter);


