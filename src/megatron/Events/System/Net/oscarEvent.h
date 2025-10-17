#pragma once

#include "SERVICE_id.h"
#include "EVENT_id.h"
#include "epoll_socket_info.h"
#include "event_mt.h"
#include "route_t.h"
#include "genum.hpp"
namespace ServiceEnum
{
    const SERVICE_id Oscar(genum_Oscar);
    // const SERVICE_id OscarSecure(genum_OscarSecure);
}

namespace oscarEventEnum
{

    const EVENT_id SendPacket(genum_oscar_SendPacket);
    const EVENT_id AddToListenTCP(genum_oscar_AddToListenTCP);
    const EVENT_id Connect(genum_oscar_Connect);
    const EVENT_id PacketOnAcceptor(genum_oscar_PacketOnAcceptor);
    const EVENT_id PacketOnConnector(genum_oscar_PacketOnConnector);
    const EVENT_id Connected(genum_oscar_Connected);
    const EVENT_id Disconnected(genum_oscar_Disconnected);
    const EVENT_id Accepted(genum_oscar_Accepted);
    const EVENT_id Disaccepted(genum_oscar_Disaccepted);
    const EVENT_id NotifyBindAddress(genum_oscar_NotifyBindAddress);
    const EVENT_id NotifyOutBufferEmpty(genum_oscar_NotifyOutBufferEmpty);
    const EVENT_id ConnectFailed(genum_oscar_ConnectFailed);
}


namespace oscarEvent
{

/// послать евент в оскар
    class SendPacket: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        SendPacket(const REF_getter<epoll_socket_info>& _esi, const REF_getter<refbuffer> &_buf, const route_t& r)
            :NoPacked(oscarEventEnum::SendPacket,r),
             esi(_esi), buf(_buf) {}

        /// сокет
        const REF_getter<epoll_socket_info> esi;

        /// buffer
        const REF_getter<refbuffer> buf;
    };

/// приход буфера на коннекторе
    class PacketOnConnector: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        PacketOnConnector(const REF_getter<epoll_socket_info> & _esi, const REF_getter<refbuffer> &_buf,const route_t &r)
            :NoPacked(oscarEventEnum::PacketOnConnector,r),
             esi(_esi),buf(_buf) {}
        /// сокет
        const REF_getter<epoll_socket_info>  esi;
        /// buffer
        const REF_getter<refbuffer> buf;
    };

/// приход буфера на слушателе
    class PacketOnAcceptor: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        PacketOnAcceptor(const REF_getter<epoll_socket_info> & _esi, const REF_getter<refbuffer> &_buf, const route_t &r)
            :NoPacked(oscarEventEnum::PacketOnAcceptor,r),
             esi(_esi),buf(_buf) {}
        /// сокет
        const REF_getter<epoll_socket_info>  esi;
        /// buffer
        const REF_getter<refbuffer> buf;
    };

    class NotifyOutBufferEmpty: public Event::NoPacked
    {
    public:
        NotifyOutBufferEmpty(const REF_getter<epoll_socket_info> &__S, const route_t& r)
            :NoPacked(oscarEventEnum::NotifyOutBufferEmpty,r), esi(__S)
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
        NotifyBindAddress(const msockaddr_in & __S,const char* _socketDescription,bool _rebind, const route_t&r)
            :NoPacked(oscarEventEnum::NotifyBindAddress,r),
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
            :NoPacked(oscarEventEnum::Disconnected,r),
             esi(__S),reason(_reason)
        {
        }
        const REF_getter<epoll_socket_info>  esi;
        const std::string reason;
    };

    class Disaccepted: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Disaccepted(const REF_getter<epoll_socket_info>  & __S,const std::string& _reason,const route_t&r)
            :NoPacked(oscarEventEnum::Disaccepted,r),
             esi(__S),reason(_reason)
        {
        }
        const REF_getter<epoll_socket_info>  esi;
        const std::string reason;
    };

    class Connected: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Connected(const REF_getter<epoll_socket_info>  & __S,const route_t&r)
            :NoPacked(oscarEventEnum::Connected,r),
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
        ConnectFailed(const REF_getter<epoll_socket_info>& _esi,const msockaddr_in  &_addr,const route_t & r):
            NoPacked(oscarEventEnum::ConnectFailed,r),
            esi(_esi),addr(_addr) {}
        const REF_getter<epoll_socket_info> esi;
        const msockaddr_in  addr;
    };

    class Connect: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Connect(const SOCKET_id& _socketId, const msockaddr_in& _addr,const char* _socketDescription, const SECURE& _sec, const route_t & r):
            NoPacked(oscarEventEnum::Connect,r),
            socketId(_socketId), addr(_addr),socketDescription(_socketDescription), secure(_sec) {}
        const SOCKET_id socketId;
        const msockaddr_in addr;
        const char* socketDescription;
        const SECURE secure;
    };


    class AddToListenTCP: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        AddToListenTCP(const SOCKET_id& _socketId, const msockaddr_in& _addr,const char* _socketDescription, const SECURE& _sec, const route_t & r):
            NoPacked(oscarEventEnum::AddToListenTCP,r),
            socketId(_socketId), addr(_addr),socketDescription(_socketDescription),secure(_sec) {}
        const SOCKET_id socketId;
        const msockaddr_in addr;
        const char* socketDescription;
        SECURE secure;
    };

/// callback notification about socket is accepted
    class Accepted: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Accepted(const REF_getter<epoll_socket_info> &__S, const route_t& r):
            NoPacked(oscarEventEnum::Accepted,r), esi(__S)
        {
        }
        /// socket object
        const REF_getter<epoll_socket_info> esi;
    };
}
