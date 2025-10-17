#pragma once

#include "EVENT_id.h"
#include "SERVICE_id.h"
#include "SOCKET_id.h"
#include "epoll_socket_info.h"
#include "event_mt.h"
#include "msockaddr_in.h"
#include "route_t.h"
#include "genum.hpp"
namespace ServiceEnum
{
    const SERVICE_id Socket(genum_Socket);
    const SERVICE_id Socks5(genum_Socks5);
}

namespace socketEventEnum
{

    const EVENT_id AddToListenTCP(genum_socket_AddToListenTCP);
    const EVENT_id AddToConnectTCP(genum_socket_AddToConnectTCP);
    const EVENT_id Accepted(genum_socket_Accepted);
    const EVENT_id StreamRead(genum_socket_StreamRead);
    const EVENT_id Connected(genum_socket_Connected);
    const EVENT_id Disconnected(genum_socket_Disconnected);
    const EVENT_id Disaccepted(genum_socket_Disaccepted);
    const EVENT_id NotifyBindAddress(genum_socket_NotifyBindAddress);
    const EVENT_id NotifyOutBufferEmpty(genum_socket_NotifyOutBufferEmpty);
    const EVENT_id ConnectFailed(genum_socket_ConnectFailed);
    const EVENT_id UdpAssoc(genum_socket_UdpAssoc);
    const EVENT_id UdpAssocRSP(genum_socket_UdpAssocRSP);
    const EVENT_id UdpPacketIncoming(genum_socket_UdpPacketIncoming);
    const EVENT_id Write(genum_socket_Write);
}


namespace socketEvent
{
    class UdpAssoc: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        UdpAssoc(
            const SOCKET_id& _socketId4,
            const SOCKET_id& _socketId6,
            const msockaddr_in  &_addr4,
            const msockaddr_in  &_addr6,
            const char* _socketDescription, const route_t & r):
            NoPacked(socketEventEnum::UdpAssoc,r),
            socketId4(_socketId4),
            socketId6(_socketId6),
            addr4(_addr4),
            addr6(_addr6),
            socketDescription(_socketDescription) {}
        const SOCKET_id socketId4;
        const SOCKET_id socketId6;
        const msockaddr_in  addr4;
        const msockaddr_in  addr6;
        const char* socketDescription;
    };
    class UdpAssocRSP: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        UdpAssocRSP(
            const SOCKET_id& _socketId4,
            const SOCKET_id& _socketId6,
            const msockaddr_in  &_addr4,
            const msockaddr_in  &_addr6,
            const REF_getter<epoll_socket_info>& _esi4,
            const REF_getter<epoll_socket_info>& _esi6,
            const int& _errcode, const route_t & r):
            NoPacked(socketEventEnum::UdpAssocRSP,r),
            socketId4(_socketId4),
            socketId6(_socketId6),
            addr4(_addr4),
            addr6(_addr6),
            esi4(_esi4),
            esi6(_esi6),
            errcode(_errcode) {}
        const SOCKET_id socketId4;
        const SOCKET_id socketId6;
        const msockaddr_in  addr4;
        const msockaddr_in  addr6;
        const REF_getter<epoll_socket_info> esi4;
        const REF_getter<epoll_socket_info> esi6;
        const int errcode;
    };

    class UdpPacketIncoming: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        UdpPacketIncoming(const REF_getter<epoll_socket_info>& _esi,const msockaddr_in  &_addrFrom, const int& _errcode, const std::string& _data, const route_t & r):
            NoPacked(socketEventEnum::UdpPacketIncoming,r),
            esi(_esi),addrFrom(_addrFrom),errcode(_errcode),data(_data) {}
        const REF_getter<epoll_socket_info> esi;
        const msockaddr_in  addrFrom;
        const int errcode;
        const std::string data;
    };

    class Write: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Write(const SOCKET_id & _socketId,const std::string& _buf)
            :NoPacked(socketEventEnum::Write),
             socketId(_socketId),buf(_buf)
        {
        }
        const SOCKET_id socketId;
        const std::string buf;
    };

    class StreamRead: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        StreamRead(const REF_getter<epoll_socket_info>  & esi_, const route_t& r)
            :NoPacked(socketEventEnum::StreamRead,r),
             esi(esi_)
        {
        }
        const REF_getter<epoll_socket_info>  esi;
    };

    class NotifyOutBufferEmpty: public Event::NoPacked
    {
    public:
        NotifyOutBufferEmpty(const REF_getter<epoll_socket_info> &__S, const route_t& r):
            NoPacked(socketEventEnum::NotifyOutBufferEmpty,r), esi(__S)
        {
        }
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        const REF_getter<epoll_socket_info> esi;
    };

    class NotifyBindAddress: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        NotifyBindAddress(const msockaddr_in & __S,const char* _socketDescription,bool _rebind,const route_t&r)
            :NoPacked(socketEventEnum::NotifyBindAddress,r),
             addr(__S),socketDescription(_socketDescription),rebind(_rebind)
        {
        }
        const msockaddr_in  addr;
        const char* socketDescription;
        const bool rebind;
    };

    class Disconnected: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Disconnected(const REF_getter<epoll_socket_info>  & __S,const std::string& _reason,const route_t&r)
            :NoPacked(socketEventEnum::Disconnected,r),
             esi(__S),reason(_reason)
        {
        }
        REF_getter<epoll_socket_info>  esi;
        std::string reason;

    };

    class Disaccepted: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Disaccepted(const REF_getter<epoll_socket_info>  & __S,const std::string& _reason,const route_t&r)
            :NoPacked(socketEventEnum::Disaccepted,r),
             esi(__S),reason(_reason)
        {
        }
        REF_getter<epoll_socket_info>  esi;
        std::string reason;

    };

    class Connected: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Connected(const REF_getter<epoll_socket_info>  & __S,const route_t&r)
            :NoPacked(socketEventEnum::Connected,r),
             esi(__S)
        {
        }
        const REF_getter<epoll_socket_info>  esi;

    };
    class ConnectFailed: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        ConnectFailed(const REF_getter<epoll_socket_info>& _esi,const msockaddr_in  &_addr,int _errno_,const route_t & r):
            NoPacked(socketEventEnum::ConnectFailed,r),
            esi(_esi),
            addr(_addr),_errno(_errno_) {}
        REF_getter<epoll_socket_info> esi;
        const msockaddr_in  addr;
        int _errno;
    };


    class AddToListenTCP: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        AddToListenTCP(const SOCKET_id& _socketId,const msockaddr_in  &_addr,const char* _socketDescription,const bool& _rebind, const SECURE& _secure, const route_t & r):
            NoPacked(socketEventEnum::AddToListenTCP,r),
            socketId(_socketId),addr(_addr),socketDescription(_socketDescription),rebind(_rebind),secure(_secure) {}
        const SOCKET_id socketId;
        const msockaddr_in  addr;
        const char* socketDescription;
        const bool rebind;
        const SECURE secure;
    };
    class AddToConnectTCP: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        AddToConnectTCP(const SOCKET_id& _socketId,const msockaddr_in  &_addr, const char* _socketDescription, const SECURE& _secure, const route_t & r):
            NoPacked(socketEventEnum::AddToConnectTCP,r),
            socketId(_socketId),addr(_addr),socketDescription(_socketDescription),secure(_secure) {}
        const SOCKET_id socketId;
        const msockaddr_in  addr;
        const char* socketDescription;
        const SECURE secure;
    };
    class Accepted: public Event::NoPacked
    {
    public:
        Accepted(const REF_getter<epoll_socket_info> &__S, const route_t& r):
            NoPacked(socketEventEnum::Accepted,r), esi(__S)
        {
        }
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        const REF_getter<epoll_socket_info> esi;
    };
}

