#pragma once
#include <quickjs.h>
#include <map>
#include <vector>
#include <string>
#include <set>
#include "common/jsscope.h"
#include "common/js_tools.h"
#include "jHolder.h"


struct EventEmitter: public Refcountable
{
    int maxListeners=10;
    std::map<std::string, std::set<JHolder>> m_listeners;
    JSContext* ctx;
    const char *comment;
    EventEmitter(JSContext* _ctx, const char* _comment) : ctx(_ctx),  comment(_comment)
    {
        MUTEX_INSPECTOR;
        DBG(iUtils->mem_add_ptr("EventEmitter",this));

        DBG(logErr2("EventEmitter::EventEmitter() %p %s", this, comment));
    }

    int emit(const std::string& event, int argc, JSValue *argv)
    {
        MUTEX_INSPECTOR;
        int res=0;
        {
            auto it = m_listeners.find(event);
            if (it != m_listeners.end()) {
                for (const auto& listener : it->second) {
                    res++;
                    auto ret=JS_Call(ctx, listener.listener, JS_UNDEFINED, argc, argv);
                    qjs::checkForException(ctx,ret,"EventEmitter:emit::JSCall");
                    JS_FreeValue(ctx, ret);
                }
            }
            else
            {
                logErr2("no listeners for event %s",event.c_str());
            }
        }
        return res;
    }

    void on(const std::string& event, JSValue listener)
    {
        MUTEX_INSPECTOR;
        if(m_listeners[event].size() >= maxListeners) {
            throw CommonError( "EventEmitter: too many listeners for event %s", event.c_str());
        }
        m_listeners[event].insert(JHolder(ctx,listener));
    }
    std::vector<JSValue> listeners(const std::string& event)
    {
        MUTEX_INSPECTOR;
        std::vector<JSValue> ret;
        {
            auto it = m_listeners.find(event);
            if (it != m_listeners.end()) {
                for(auto &z: it->second)
                {
                    ret.push_back(z.listener);
                }
            }
        }
        return ret;

    }

    ~EventEmitter() {
        DBG(iUtils->mem_remove_ptr("EventEmitter",this));

        DBG(logErr2("EventEmitter::~EventEmitter() %p %s %s",this,comment,_DMI().c_str()));
    }

};
JSValue new_event_emitter(JSContext *ctx,const REF_getter<EventEmitter>& emitter);


