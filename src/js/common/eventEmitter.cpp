#include <string>
#include <vector>
#include "eventEmitter.h"

int EventEmitter::emit(const std::string& event, int argc, JSValue *argv)
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
void EventEmitter::on(const std::string& event, JSValue listener)
{
    MUTEX_INSPECTOR;
    if(m_listeners[event].size() >= maxListeners) {
        throw CommonError( "EventEmitter: too many listeners for event %s", event.c_str());
    }
    m_listeners[event].insert(JHolder(ctx,listener));
}

std::vector<JSValue> EventEmitter::listeners(const std::string& event)
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
