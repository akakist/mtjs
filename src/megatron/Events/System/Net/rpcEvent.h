#pragma once

#include "EVENT_id.h"
#include "SERVICE_id.h"
#include "epoll_socket_info.h"
#include "event_mt.h"
#include "genum.hpp"
#include "IUtils.h"
namespace ServiceEnum
{
    const SERVICE_id RPC(genum_RPC);
    const SERVICE_id ObjectProxyPolled(genum_ObjectProxyPolled);
    const SERVICE_id ObjectProxyThreaded(genum_ObjectProxyThreaded);

}

namespace rpcEventEnum
{

    const EVENT_id PassPacket(genum_rpcPassPacket);
    const EVENT_id SendPacket(genum_rpcSendPacket);
    const EVENT_id SubscribeNotifications(genum_rpcSubscribeNotifications);
    const EVENT_id UnsubscribeNotifications(genum_rpcUnsubscribeNotifications);
    const EVENT_id Disconnected(genum_rpcDisconnected);
    const EVENT_id Disaccepted(genum_rpcDisaccepted);
    const EVENT_id ConnectFailed(genum_rpcConnectFailed);
    const EVENT_id Connected(genum_rpcConnected);
    const EVENT_id Accepted(genum_rpcAccepted);
    const EVENT_id Binded(genum_rpcBinded);
    const EVENT_id IncomingOnAcceptor(genum_rpcIncomingOnAcceptor);
    const EVENT_id IncomingOnConnector(genum_rpcIncomingOnConnector);
    const EVENT_id NotifyOutBufferEmpty(genum_rpc_NotifyOutBufferEmpty);
    const EVENT_id DoListen(genum_rpc_DoListen);
}




namespace rpcEvent {
    class UnsubscribeNotifications: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        UnsubscribeNotifications(const route_t & r)
            :NoPacked(rpcEventEnum::UnsubscribeNotifications,r)
        {}
    };
    class SubscribeNotifications: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        SubscribeNotifications(const route_t & r)
            :NoPacked(rpcEventEnum::SubscribeNotifications,r)
        {}
    };


    class SendPacket: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        SendPacket(const msockaddr_in& _addrTo, const SERVICE_id& dst,const REF_getter<Event::Base>& _ev)
            :NoPacked(rpcEventEnum::SendPacket),
             addressTo(_addrTo),destination(dst), ev(_ev) {}
        const msockaddr_in addressTo;
        const SERVICE_id destination;
        const REF_getter<Event::Base> ev;

    };

    class PassPacket: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        PassPacket(const SOCKET_id& _addrTo, const REF_getter<Event::Base>  &_e)
            :NoPacked(rpcEventEnum::PassPacket),
             socketIdTo(_addrTo),e(_e) {}
        const SOCKET_id socketIdTo;
        const REF_getter<Event::Base> e;

    };

    class NotifyOutBufferEmpty: public Event::NoPacked
    {
    public:
        NotifyOutBufferEmpty(const route_t& r)
            :NoPacked(rpcEventEnum::NotifyOutBufferEmpty,r)
        {
        }
        static Base* construct(const route_t &)
        {
            return NULL;
        }
    };

    class IncomingOnConnector: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        IncomingOnConnector(const REF_getter<epoll_socket_info>&_esi,
                            const msockaddr_in& _addr,const REF_getter<Event::Base> & _e)
            :NoPacked(rpcEventEnum::IncomingOnConnector),
             esi(_esi),connect_addr(_addr),e(_e) {}
        const REF_getter<epoll_socket_info> esi;
        const msockaddr_in connect_addr;
        const REF_getter<Event::Base> e;
    };


    class IncomingOnAcceptor: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        IncomingOnAcceptor(const REF_getter<epoll_socket_info>&_esi,const REF_getter<Event::Base> & _e)
            :NoPacked(rpcEventEnum::IncomingOnAcceptor),
             esi(_esi),e(_e) {}
        const REF_getter<epoll_socket_info> esi;
        const REF_getter<Event::Base> e;
    };

    class Disconnected: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Disconnected(const REF_getter<epoll_socket_info>& _esi, const msockaddr_in & _addr, const std::string& _reason, const route_t &r)
            :NoPacked(rpcEventEnum::Disconnected,r),
             esi(_esi),destination_addr(_addr), reason(_reason) {}
        const REF_getter<epoll_socket_info> esi;
        const msockaddr_in destination_addr;
        const std::string  reason;
    };


    class Disaccepted: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Disaccepted(const REF_getter<epoll_socket_info>& _esi, const std::string& _reason, const route_t &r)
            :NoPacked(rpcEventEnum::Disaccepted,r),
             esi(_esi),reason(_reason) {}
        const REF_getter<epoll_socket_info> esi;
        const std::string  reason;

    };

    class Connected: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Connected(const REF_getter<epoll_socket_info>& _esi, const msockaddr_in & _addr, const route_t &r)
            :NoPacked(rpcEventEnum::Connected,r),
             esi(_esi),addr(_addr) {}
        const REF_getter<epoll_socket_info> esi;
        const msockaddr_in addr;

    };


    class ConnectFailed: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        ConnectFailed(const msockaddr_in & _addr, const route_t &r)
            :NoPacked(rpcEventEnum::ConnectFailed,r),
             destination_addr(_addr) {}
        const msockaddr_in destination_addr;
    };

    class Binded: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Binded(const REF_getter<epoll_socket_info>& _esi)
            :NoPacked(rpcEventEnum::Binded),
             esi(_esi) {}
        const REF_getter<epoll_socket_info>  esi;
    };

    class Accepted: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Accepted(const REF_getter<epoll_socket_info>& _esi, const route_t &r)
            :NoPacked(rpcEventEnum::Accepted,r),
             esi(_esi) {}
        const REF_getter<epoll_socket_info> esi;
    };
    class DoListen: public Event::NoPacked
    {
        /**
        request HTTP to listen port
        line like "INADDR_ANY:8080:128" - ip:port:listenBacklog
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        DoListen(const msockaddr_in& _addr, const SECURE& _sec)
            :NoPacked(rpcEventEnum::DoListen),
             addr(_addr), secure(_sec) {}
        const msockaddr_in addr;
        const SECURE secure;

    };


}
