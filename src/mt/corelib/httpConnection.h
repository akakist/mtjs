#pragma once
#include <map>
#include <vector>
#include <string>
#include "REF.h"
#include "epoll_socket_info.h"
#include <deque>
#include "IInstance.h"
#include "stream.h"
#include "llhttp/llhttp.h"
#include "Events/System/Net/socketEvent.h"
/**
    HTTP::Request, HTTP::Response is used in http service
*/


typedef int (*__url_read)(long _fd, unsigned char *bufferForOutput, int _bufSizeRequested);
typedef int64_t (*__url_seek)(long _fd, int64_t pos, int whence);
typedef int (*__url_close)(long _fd);
typedef long (*__url_open)(const char* fn, int flags);

struct HttpContext: public Refcountable
{
    std::string method;
    int method_int=HTTP_GET;
    std::string url;

    std::string current_field;
    std::string current_value;
    std::map<std::string, std::string> headers;

    bool keepalive = true;
    bool upgrade = false;
    REF_getter<refbuffer> body=nullptr;
    size_t chunk_size = 0;
    bool is_chunked=false;
    int chunkId=0;
    void *server;
    bool isWebSocket=false;
    std::string websocket_buffer;
    size_t content_length=0;
    REF_getter<epoll_socket_info> esi;

private:
    REF_getter<Stream> reader=nullptr;
public:
    void setReader(const REF_getter<Stream>& r)
    {
        reader=r;
    }
    REF_getter<Stream> getReader()
    {
        return reader;
    }
    // std::string post_content;


    void clear() ;
    HttpContext(void *_server, const REF_getter<epoll_socket_info>& _esi):Refcountable("HttpContext"),
        server(_server),esi(_esi) {}
};


namespace HTTP
{
    class Service;
    const char* get_name_of_http_code(int code);

    class IoProtocol
    {
    public:
        __url_open m_open;
        __url_read m_read;
        __url_seek m_seek;
        __url_close m_close;
        IoProtocol():m_open(NULL),m_read(NULL),m_seek(NULL),m_close(NULL)
        {

        }

    };
    enum CONN_TYPE
    {
        CONN_CLOSE, CONN_KEEP_ALIVE, CONN_UPGRADE,CONN_UNKNOWN
    };


    class Request:public Refcountable
    {

    public:

        llhttp_t parser;
        REF_getter< HttpContext>  ctx=nullptr;




        Request(const REF_getter<epoll_socket_info>& _esi, llhttp_settings_t& settings, void* server);





        REF_getter<socketEvent::StreamRead> currentEvent=nullptr;

#ifdef KALL
        struct _fileresponse: public Refcountable
        {
            bool hasRange;
            bool headerSent;
            int64_t written_bytes;
            int64_t startb, endb, contentLength;
            std::string fileName;
            std::string extension;

        private:
            long m_fd;
            bool m_closed;
        public:
            void set_fd(const long &fd) {
                m_fd=fd;
            }
            long get_fd()
            {
                return m_fd;
            }
            int64_t fileSize;
            IoProtocol io_protocol;

            bool closed()
            {
                return m_closed;
            }
            void close()
            {
                m_closed=true;
            }

            _fileresponse():hasRange(false),headerSent(false),written_bytes(0),startb(0),endb(0),contentLength(0),
                m_fd(-1),m_closed(false),fileSize(-1LL) {}
            ~_fileresponse();
        };
        REF_getter<_fileresponse> fileresponse;
#endif
        // enum CONN {
        //     CLOSE,KEEPALIVE,UPGRADE
        // };
        // CONN connection=CLOSE;
        // bool sendRequestIncomingIsSent;
        REF_getter<epoll_socket_info> esi;

        ///   websocket data


    private:
    };
    class Response
    {
    public:
        Response(const REF_getter<HttpContext> &rq):
            request(rq),http_code(200),http_content_type("text/html"),is_chunked(false),chunked_http_header_sent(false)
        {

        }
    public:
        REF_getter<HttpContext> request=nullptr;
    private:
        int http_code;
        std::string http_content_type;
        std::string content;
        std::string http_charset;
        std::string serverName;
        std::string http_header_out;
        // std::map<std::string,std::string> out_cookies;
        // bool allow_build_response;
        bool chunked_http_header_sent=false;
        CONN_TYPE connection=HTTP::CONN_CLOSE;
        std::string build_html_response();
        // void make_response();
        // void chunkedOut(const std::string & data);
        // void chunkedClose();
    public:
        bool is_chunked=false;
        REF_getter<Stream> writer=nullptr;
        void setHeader(const std::string &key, const std::string &value)
        {
            if(!strcasecmp(key.c_str(),"Connection"))
            {
                if(!strcasecmp(value.c_str(),"Close"))
                    connection=HTTP::CONN_CLOSE;
                else if(!strcasecmp(value.c_str(),"Keep-Alive"))
                    connection=HTTP::CONN_KEEP_ALIVE;
                else if(!strcasecmp(value.c_str(),"Upgrade"))
                    connection=HTTP::CONN_UPGRADE;
            }
            else
                http_header_out+= key + ": "+ value + "\r\n";
        }
        void setContentType(const std::string &ct)
        {
            http_content_type=ct;
        }
        void setStatus(int code)
        {
            http_code=code;
        }
        void make_response(const std::string& s);
        void send_chunked(const std::string& s);
        void end_chunked();



        ~Response()
        {
            // try{
            //     if(content.size())
            //     {
            //         logErr2("content.size() %s call end()",content.c_str());
            //     }

            // }
            // catch(...)
            // {
            //     logErr2("Response destructor exception");
            // }
        }

        // IInstance *iInstance;
    };

};

