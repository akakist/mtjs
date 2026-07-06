#pragma once
#include "epoll_socket_info.h"
#include "ghash.h"
namespace ServiceEnum
{
    const SERVICE_id WebSocketClient(ghash("@g_WebSocketClient"));

}

namespace webSocketClientEventEnum
{

    const EVENT_id Connect(ghash("@g_wscConnect"));
    const EVENT_id Connected(ghash("@g_wscConnected"));
    const EVENT_id Disconnected(ghash("@g_wscDisconnected"));
    const EVENT_id Send(ghash("@g_wscSend"));
    const EVENT_id Received(ghash("@g_wscReceived"));

    const EVENT_id ConnectFailed(ghash("@g_wscConnectFailed"));
}

namespace webSocketClientEvent
{
    class Connect: public Event::NoPacked
    {
    public:


        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Connect(
            const SOCKET_id& _socketId,
            const std::string & url_,
            const char* _socketDescription, const route_t & r):
            NoPacked(webSocketClientEventEnum::Connect,r),
            socketId(_socketId),
            url(url_),
            socketDescription(_socketDescription) {}
        const SOCKET_id socketId;
        const std::string url;
        const char* socketDescription;
    };
    class Send: public Event::NoPacked
    {
    public:

        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Send(
            const REF_getter<epoll_socket_info> _esi,
            const std::string  &_msg,
            const route_t & r):
            NoPacked(webSocketClientEventEnum::Send,r),
            esi(_esi),
            msg(_msg)
        {}
        const REF_getter<epoll_socket_info> esi;
        const std::string msg;
    };
    class Received: public Event::NoPacked
    {
    public:

        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Received(
            const REF_getter<epoll_socket_info>& _esi,
            const std::string  &_msg,
            const route_t & r):
            NoPacked(webSocketClientEventEnum::Received,r),
            esi(_esi),
            msg(_msg)
        {}
        const REF_getter<epoll_socket_info> esi;
        const std::string msg;
    };

    class Connected: public Event::NoPacked
    {
    public:

        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Connected(
            const REF_getter<epoll_socket_info>  &_esi,
            const route_t & r):
            NoPacked(webSocketClientEventEnum::Connected,r),
            esi(_esi)
        {}
        const REF_getter<epoll_socket_info> esi;
    };
    class ConnectFailed: public Event::NoPacked
    {
    public:

        static Base* construct(const route_t &)
        {
            return NULL;
        }
        ConnectFailed(
            const REF_getter<epoll_socket_info>  &_esi,
            const route_t & r):
            NoPacked(webSocketClientEventEnum::ConnectFailed,r),
            esi(_esi)
        {}
        const REF_getter<epoll_socket_info> esi;
    };

    class Disconnected: public Event::NoPacked
    {
    public:

        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Disconnected(
            const REF_getter<epoll_socket_info>  &_esi,
            const route_t & r):
            NoPacked(webSocketClientEventEnum::Disconnected,r),
            esi(_esi)
        {}
        const REF_getter<epoll_socket_info> esi;
    };

}