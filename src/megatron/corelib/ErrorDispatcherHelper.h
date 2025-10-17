#pragma once
#include "Events/Tools/errorDispatcherEvent.h"
#include "IInstance.h"
#include <stdarg.h>
class ErrorDispatcherHelper
{
    IInstance* ifa;
public:
    ErrorDispatcherHelper(IInstance* _ifa): ifa(_ifa) {}
    void errorDispatcherSendMessage(int edcode, const char* fmt, ...)
    {
        va_list ap;
        char str[1024*10];
        va_start(ap, fmt);
        vsnprintf(str, sizeof(str), fmt, ap);
        va_end(ap);
        ifa->sendEvent(ServiceEnum::ErrorDispatcher,new errorDispatcherEvent::SendMessage(edcode,str));
    }
    void errorDispatcherSendMessage(const std::string & remoteAddress, int edcode, const char* fmt, ...)
    {
        va_list ap;
        char str[1024*10];
        va_start(ap, fmt);
        vsnprintf(str, sizeof(str), fmt, ap);
        va_end(ap);
        ifa->sendEvent(remoteAddress,ServiceEnum::ErrorDispatcher,new errorDispatcherEvent::SendMessage(edcode,str));
    }

};

