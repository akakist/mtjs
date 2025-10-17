#pragma once

#include "unknown.h"

#include "broadcaster.h"
#include "listenerSimple.h"
#include "httpConnection.h"

#include "unknown.h"
#include <Events/System/Net/socketEvent.h>
#include <Events/System/Net/httpEvent.h>
#include <Events/System/Run/startServiceEvent.h>




namespace HTTP
{

    enum WebSocketFrameType {
        ERROR_FRAME=0xFF00,
        INCOMPLETE_FRAME=0xFE00,

        OPENING_FRAME=0x3300,
        CLOSING_FRAME=0x3400,

        INCOMPLETE_TEXT_FRAME=0x01,
        INCOMPLETE_BINARY_FRAME=0x02,

        TEXT_FRAME=0x81,
        BINARY_FRAME=0x82,

        PING_FRAME=0x19,
        PONG_FRAME=0x1A
    };

    class Service:
        public UnknownBase,
        public Broadcaster,
        public ListenerSimple
    {

        // config
        size_t m_maxPost;
        struct _mx
        {
            RWLock lk;
            // std::set<std::string> docUrls;
            // std::string documentRoot;
            std::map<std::string,std::string>mime_types;
            // std::map<std::string,HTTP::IoProtocol> protocols;
            std::set<msockaddr_in> bind_addrs;
        };
        _mx mx;
        ListenerBase* socketListener;
        //!config
        bool on_StreamRead(const socketEvent::StreamRead* evt);
        bool on_Accepted(const socketEvent::Accepted* evt);
        bool on_Connected(const socketEvent::Connected*);
        bool on_NotifyBindAddress(const socketEvent::NotifyBindAddress*);
        bool on_Disaccepted(socketEvent::Disaccepted*);
        bool on_Disconnected(socketEvent::Disconnected*);

        bool on_WSWrite(const httpEvent::WSWrite*);
        bool on_DoListen(const httpEvent::DoListen*);


        bool on_startService(const systemEvent::startService*);
        bool on_NotifyOutBufferEmpty(const socketEvent::NotifyOutBufferEmpty* );
        bool on_GetBindPortsREQ(const httpEvent::GetBindPortsREQ*);

        REF_getter<HTTP::Request> getData(epoll_socket_info* esi);
        void setData(epoll_socket_info* esi, const REF_getter<HTTP::Request> & p);
        void clearData(epoll_socket_info* esi);
        bool handleChunkedBuffer(const socketEvent::StreamRead* evt, const REF_getter<HTTP::Request>& W);


        static HTTP::WebSocketFrameType WebSocket_getFrame(unsigned char* in_buffer, int in_length, /*unsigned char* out_buffer, int out_size, int* out_length,*/ std::string &o);
        static std::string WebSocket_makeFrame(WebSocketFrameType frame_type, unsigned char* msg, int msg_length/*, unsigned char* buffer, int buffer_size*/);

    public:
        void deinit()
        {
            ListenerSimple::deinit();
        }
        static UnknownBase*construct(const SERVICE_id&id, const std::string&nm, IInstance *_if);
        Service(const SERVICE_id& id, const std::string& nm, IInstance *_if);
        ~Service() {
        }


        std::string get_mime_type(const std::string& mime) const;

    protected:
        bool handleEvent(const REF_getter<Event::Base>& evt);
        /** -1 error*/
        int send_other_from_disk_ext(const REF_getter<epoll_socket_info>&esi, const REF_getter<HTTP::Request>&req,const std::string & fn,const std::string& exten);

    private:
        struct _lastModified: public Mutexable
        {
            std::map<std::string_view,time_t> container;
        };
        _lastModified lastModified;


        IInstance *iInstance;
    };
};
