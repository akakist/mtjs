#pragma once
#include <map>
#include <vector>
#include <string>
#include "REF.h"
#include "epoll_socket_info.h"
#include <deque>
#include "IInstance.h"
#include "stream.h"

#include "ioStreams.h"

// #include "streamBase.h"
/**
    HTTP::Request, HTTP::Response is used in http service
*/


typedef int (*__url_read)(long _fd, unsigned char *bufferForOutput, int _bufSizeRequested);
typedef int64_t (*__url_seek)(long _fd, int64_t pos, int whence);
typedef int (*__url_close)(long _fd);
typedef long (*__url_open)(const char* fn, int flags);


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

    struct token
    {
        size_t pz;
        size_t len;
    };
#define LINE0TKSIZE 10
    enum CONN_TYPE
    {
        CONN_CLOSE, CONN_KEEP_ALIVE, CONN_UPGRADE,CONN_UNKNOWN
    };
    struct header_params_
    {
        token HOST;
        token USER_AGENT;
        token ACCEPT;
        token ACCEPT_LANGUAGE;
        token ACCEPT_ENCODING;
        CONN_TYPE CONNECTION=HTTP::CONN_CLOSE;
        token CONTENT_TYPE;
        token UPGRADE;
        int CONTENT_LENGTH;
        token COOKIE;
        token REFERER;
        token AUTHORIZATION;
        token TRANSFER_ENCODING;
        token RANGE;
        token Sec_WebSocket_Key;
        token Sec_WebSocket_Version;
        token EXPECT;

    };
    enum METHOD
    {
        METHOD_NOTSET,METHOD_GET, METHOD_POST, METHOD_PUT, METHOD_DELETE
    };

    struct http_header_parse_data
    {

        METHOD method;
        token uri;
        token http_version;
        header_params_ header_params;
        size_t cur_pos;
        size_t cur_line;
        size_t cur_line_size;
        size_t cur_line_start;
        size_t post_start;
        bool done_header;
        char last_char;
        token line0tk[LINE0TKSIZE];
        size_t line0tk_idx;
        void dump(const char* req, int reqsize);

    };
    class Request:public Refcountable
    {

    public:
        void parse(const char* req, int reqsize);
        http_header_parse_data parse_data;

        std::string_view uri()
        {
            return  tosv_h(parse_data.uri);
        }
        std::string_view headers()
        {
            return  header_content;
        }
        std::string_view tosv_h(const token& t)
        {
            return {&header_content[t.pz],t.len};
        }
        std::string_view tosv_c(const token& t)
        {
            return {&post_content[t.pz],t.len};
        }
        // std::string_view postContent;
        bool chunked=false;

        std::string header_content;
        std::string post_content;

        Request(const REF_getter<epoll_socket_info>& _esi);


        time_t m_last_io_time;
        bool websocket_established=false;

        REF_getter<Stream> reader=nullptr;
        struct _mx_buffer
        {
        };
        _mx_buffer mx_chunked;
        int chunkId=0;


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
        // enum CONN {
        //     CLOSE,KEEPALIVE,UPGRADE
        // };
        // CONN connection=CLOSE;
        bool sendRequestIncomingIsSent;
        REF_getter<epoll_socket_info> esi;

        ///   websocket data
        bool isWebSocket=false;
        std::string websocket_buffer;


    private:
    };
    class Response
    {
    public:
        Response(const REF_getter<HTTP::Request> &rq):
            request(rq),http_code(200),http_content_type("text/html"),is_chunked(false),chunked_http_header_sent(false)
        {

        }
    public:
        REF_getter<Request> request=nullptr;
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
        // void makeResponse();
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

