#pragma once

#include "EVENT_id.h"
#include "IUtils.h"
#include "SERVICE_id.h"
#include "httpConnection.h"
#include "route_t.h"
namespace ServiceEnum
{
    const SERVICE_id HTTP(genum_HTTP);
}
namespace httpEventEnum
{
    const EVENT_id DoListen(genum_httpDoListen);
    const EVENT_id GetBindPortsREQ(genum_GetBindPortsREQ);
    const EVENT_id GetBindPortsRSP(genum_GetBindPortsRSP);
    const EVENT_id WSTextMessage(genum_WSTextMessage);
    const EVENT_id WSDisaccepted(genum_WSDisaccepted);
    const EVENT_id WSDisconnected(genum_WSDisconnected);
    const EVENT_id WSWrite(genum_WSWrite);
    
    const EVENT_id RequestIncoming(genum_httpRequestIncoming);
    const EVENT_id RequestStartChunking(genum_httpRequestStartChunking);
    const EVENT_id RequestChunkReceived(genum_httpRequestChunkReceived);
    const EVENT_id RequestChunkingCompleted(genum_httpRequestChunkingCompleted);

}

namespace httpEvent
{
    class WSDisaccepted: public Event::NoPacked
    {
        /**
        request is ready, sent by HTTP
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        WSDisaccepted(const REF_getter<HTTP::Request> _rq, const route_t & r)
            :NoPacked(httpEventEnum::WSDisaccepted,r),
             req(_rq){}
        const REF_getter<HTTP::Request> req;

    };
    class WSWrite: public Event::NoPacked
    {
        /**
        request is ready, sent by HTTP
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        WSWrite(const REF_getter<HTTP::Request>& _r, const std::string & _msg, const route_t & r)
            :NoPacked(httpEventEnum::WSWrite,r),
             r(_r),msg(_msg){}
        REF_getter<HTTP::Request> r=nullptr;
        std::string msg;

    };
    class WSDisconnected: public Event::NoPacked
    {
        /**
        request is ready, sent by HTTP
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        WSDisconnected(const REF_getter<HTTP::Request> & _rq, const route_t & r)
            :NoPacked(httpEventEnum::WSDisconnected,r),
             req(_rq){}
        const REF_getter<HTTP::Request> req;

    };
    class WSTextMessage: public Event::NoPacked
    {
        /**
        request is ready, sent by HTTP
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        WSTextMessage(const REF_getter<HTTP::Request>& __R, const std::string& _msg, const route_t & r)
            :NoPacked(httpEventEnum::WSTextMessage,r),
             req(__R), msg(_msg){}
        const REF_getter<HTTP::Request> req;
        std::string msg;

    };
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
        RequestIncoming(const REF_getter<HTTP::Request>& __R, const REF_getter<epoll_socket_info>& __esi, const route_t & r)
            :NoPacked(httpEventEnum::RequestIncoming,r),
             req(__R),esi(__esi) {}
        const REF_getter<HTTP::Request> req;
        const REF_getter<epoll_socket_info> esi;

    };





    class GetBindPortsREQ: public Event::Base
    {


    public:
        static Base* construct(const route_t &r)
        {
            return new GetBindPortsREQ(r);
        }
        GetBindPortsREQ(const route_t& r)
            :Base(httpEventEnum::GetBindPortsREQ,r) {}

        void unpack(inBuffer& )
        {
        }
        void pack(outBuffer&) const
        {
        }
    };
    class GetBindPortsRSP: public Event::Base
    {


    public:
        static Base* construct(const route_t &r)
        {
            return new GetBindPortsRSP(r);
        }
        GetBindPortsRSP(const route_t& r)
            :Base(httpEventEnum::GetBindPortsRSP,r) {}
        GetBindPortsRSP(const std::set<msockaddr_in>& _ListenAdr,  const route_t &r)
            :Base(httpEventEnum::GetBindPortsRSP,r),
             listenAddrs(_ListenAdr) {}
        std::set<msockaddr_in> listenAddrs;

        void unpack(inBuffer& o)
        {
            o>>listenAddrs;
        }
        void pack(outBuffer&o) const
        {
            o<<listenAddrs;
        }

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
        DoListen(const msockaddr_in& _addr, const SECURE& _sec, const route_t & r)
            :NoPacked(httpEventEnum::DoListen,r),
             addr(_addr), secure(_sec) {}
        const msockaddr_in addr;
        const SECURE secure;

    };



    class RequestStartChunking: public Event::NoPacked
    {
        /**
        request is ready, sent by HTTP
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        RequestStartChunking(const REF_getter<HTTP::Request>& __R, const REF_getter<epoll_socket_info>& __esi, const route_t & r)
            :NoPacked(httpEventEnum::RequestStartChunking,r),
             req(__R),esi(__esi) {}
        const REF_getter<HTTP::Request> req;
        const REF_getter<epoll_socket_info> esi;

    };

    class RequestChunkReceived: public Event::NoPacked
    {
        /**
        request is ready, sent by HTTP
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        RequestChunkReceived(const REF_getter<HTTP::Request>& __R, const REF_getter<epoll_socket_info>& __esi, uint64_t _chunkId, const std::string& _buf, const route_t & r)
            :NoPacked(httpEventEnum::RequestChunkReceived,r),
             req(__R),esi(__esi),chunkId(_chunkId),buf(_buf){}
        const REF_getter<HTTP::Request> req;
        const REF_getter<epoll_socket_info> esi;
        uint64_t chunkId=0;
        std::string buf;

    };

    class RequestChunkingCompleted: public Event::NoPacked
    {
        /**
        request is ready, sent by HTTP
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        RequestChunkingCompleted(const REF_getter<HTTP::Request>& __R, const REF_getter<epoll_socket_info>& __esi, uint64_t _totalChunks, const route_t & r)
            :NoPacked(httpEventEnum::RequestChunkingCompleted,r),
             req(__R),esi(__esi),totalChunks(_totalChunks) {}
        const REF_getter<HTTP::Request> req;
        const REF_getter<epoll_socket_info> esi;
        uint64_t totalChunks=0;

    };

}

