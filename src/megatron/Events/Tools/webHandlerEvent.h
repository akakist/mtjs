#pragma once
// #ifdef WEBDUMP
#include "EVENT_id.h"
#include "SERVICE_id.h"
#include "epoll_socket_info.h"
#include "event_mt.h"
#include "genum.hpp"
#include "httpConnection.h"
#include "route_t.h"
#include "genum.hpp"
namespace ServiceEnum
{
    const SERVICE_id WebHandler(genum_WebHandler);
}

namespace webHandlerEventEnum
{

    const EVENT_id RegisterHandler(genum_webHandlerRegisterHandler);
    const EVENT_id RegisterDirectory(genum_webHandlerRegisterDirectory);
    const EVENT_id RequestIncoming(genum_webHandlerRequestIncoming);
}

namespace webHandlerEvent
{
    class RequestIncoming: public Event::NoPacked
    {
        /**
        request is ready, sent by HTTP
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        RequestIncoming(const REF_getter<HTTP::Request>& __R,
                        const REF_getter<epoll_socket_info>& __esi,const route_t & r)
            :NoPacked(webHandlerEventEnum::RequestIncoming,r),
             req(__R),esi(__esi) {}
        const REF_getter<HTTP::Request> req;
        const REF_getter<epoll_socket_info> esi;

    };

    class RegisterHandler: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        RegisterHandler(const std::string& _url, const std::string& _displayName, const route_t & r)
            :NoPacked(webHandlerEventEnum::RegisterHandler,r),
             url(_url),displayName(_displayName) {}
        const std::string url,displayName;

    };

    class RegisterDirectory: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &)
        {
            return  NULL;
        }
        RegisterDirectory(const std::string& _url, const std::string& _displayName)
            :NoPacked(webHandlerEventEnum::RegisterDirectory),
             url(_url),displayName(_displayName) {}
        const std::string url,displayName;

    };

};
// #endif