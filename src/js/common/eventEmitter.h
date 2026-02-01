#pragma once
#include <quickjs.h>
#include <map>
#include <vector>
#include <string>
#include <set>
#include "jsscope.h"
#include "js_tools.h"
#include "jHolder.h"


struct EventEmitter: public Refcountable
{
    int maxListeners=10;
    std::map<std::string, std::set<JHolder>> m_listeners;
    JSContext* ctx;
    const char *comment;
    EventEmitter(JSContext* _ctx, const char* _comment) : ctx(_ctx),  comment(_comment)
    {
    }

    int emit(const std::string& event, int argc, JSValue *argv);

    void on(const std::string& event, JSValue listener);
    std::vector<JSValue> listeners(const std::string& event);

    ~EventEmitter() 
    {
    }

};
JSValue new_event_emitter(JSContext *ctx,const REF_getter<EventEmitter>& emitter);


