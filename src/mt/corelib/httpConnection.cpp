#include "splitStr.h"
#include <unistd.h>
#include <sstream>
#define _FILE_OFFSET_BITS 64
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <stdlib.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif
#include <string>
#include "httpConnection.h"
#include <sstream>
#include "Events/System/Net/socketEvent.h"

const char* HTTP::get_name_of_http_code(int code)
{
    std::string res;
    switch (code)
    {
    case 200:
        return "OK";
        break;
    case 203:
        return "Non-Authoritative Information";
        break;
    case 206:
        return "Partial Content";
        break;
    case 302:
        return "Found";
        break;
    case 303:
        return "See Other";
        break;
    case 400:
        return "Bad Request";
        break;
    case 401:
        return "Unauthorized";
        break;
    case 402:
        return "Payment Required";
        break;
    case 403:
        return "Forbidden";
        break;
    case 404:
        return "Not Found";
        break;
    case 405:
        return "Method Not Allowed";
        break;
    case 406:
        return "Not Acceptable";
        break;
    case 413:
        return "Request Entity Too Large";
        break;
    case 500:
        return "Internal Server Error";
        break;
    case 501:
        return "Not Implemented";
        break;
    case 502:
        return "Bad Gateway";
        break;
    case 503:
        return "Service Unavailable";
        break;
    case 505:
        return "HTTP Version not supported";
        break;
    default:
        return "Unknow";
        break;
        break;
    }
    return "Wrong CODE";
}
std::string HTTP::Response::build_html_response()
{
    std::stringstream ret;
    ret << "HTTP/1.1 " << http_code << " " << HTTP::get_name_of_http_code(http_code) << "\r\n";
    if (http_content_type != "")
    {
        std::string r = http_content_type;
        ret<<"Content-Type: " << r;
        if (http_charset.size())
            ret<< "; charset: " << http_charset;
        ret << "\r\n";

    }


    if(serverName.size())
        ret <<"Server: "<< serverName;
    else
        ret<< "Server: " << "Web Server";
    ret << "\r\n";
    if(!is_chunked && content.size())

        ret<< "Content-Length: " << content.size() << "\r\n";
    if(request->keepalive && !is_chunked)
    {
        ret << "Connection: Keep-Alive\r\n"
            "Keep-Alive: timeout=7200, max=1000000000\r\n";

    }
    else ret << "Connection: close\r\n";
    if(is_chunked)
        ret <<"Transfer-Encoding: chunked\r\n";

    if(http_header_out.size())
        ret << http_header_out;

    //     if (out_cookies.size())
    // {
    //     for (auto& i: out_cookies)
    //     {
    //         ret<< "Set-Cookie: " << i.first;
    //         ret<< "=";
    //         ret<< i.second;
    //         ret<< "; path=/\r\n";

    //     }
    // }

    ret << "\r\n";
    if(!is_chunked)
    {
        ret << content;
    }
    // logErr2("ret %s",ret.str().c_str());
    return ret.str();
}

inline int ascii_tolower(int c) {
    return (c >= 'A' && c <= 'Z') ? (c + 32) : c;
}
// Самые частые заголовки для оптимизации сравнения
#include <iostream>
#include "sv.h"


HTTP::Request::Request(const REF_getter<epoll_socket_info>& _esi, llhttp_settings_t& settings, void* server)
    :
    // m_last_io_time(time(NULL)),
    // fileresponse(NULL),
    // sendRequestIncomingIsSent(false),
    Refcountable("HTTP::Request::Request"),
    esi(_esi)
{
    // memset(& parse_data,0,sizeof(parse_data));
    // parse_data.last_char=' ';
    llhttp_init(&parser, HTTP_REQUEST, &settings);
    parser.data = this;
    ctx = new HttpContext(server, _esi);
    // ctx-.server=server;

}
void HTTP::Response::make_response(const std::string& str)
{

    if(is_chunked)
        throw CommonError("if(is_chunked)");

    // if()

    content=str;
    auto s=build_html_response();
    if( request->keepalive)
    {
        request->esi->write_(s);
    }
    else
    {
        request->esi->write_and_close(s);
    }


}

void HTTP::Response::send_chunked(const std::string& str)
{
    // logErr2("send_chunked");
    is_chunked=true;
    if(content.size())
        throw CommonError("if(content.size())");

    {
        if(!chunked_http_header_sent)
        {

            auto s=build_html_response();
            request->esi->write_(s);
            chunked_http_header_sent=true;
        }
        {
            char buf[100];

            snprintf(buf,sizeof(buf),"%lx\r\n",str.size());
            std::string o=buf+str+"\r\n";
            request->esi->write_(o);
        }
    }
}

void HTTP::Response::end_chunked()
{
    if(!is_chunked)
        throw CommonError("if(!is_chunked)");
    if(content.size())
        throw CommonError("if(content.size())");
    if(!request.valid())
        throw CommonError("if(!request->ctx.valid())");

    {
        {

            std::string buf="0\r\n\r\n";
            if( request->keepalive)
            {
                request->esi->write_(buf);
            }
            else
            {
                request->esi->write_and_close(buf);
            }
        }
    }

}


#ifdef KALL
HTTP::Request::_fileresponse::~_fileresponse()
{
    if(m_fd!=-1)
    {
        if(io_protocol.m_close)
        {
            io_protocol.m_close(m_fd);
        }
        else
        {
            ::close(m_fd);
        }
        m_fd=-1;
    }
}
#endif
void HttpContext::clear()
{
    method.clear();
    url.clear();
    current_field.clear();
    current_value.clear();
    headers.clear();
    keepalive = true;
    upgrade = false;
    body=nullptr;
    chunk_size = 0;
    is_chunked=false;
    chunkId=0;
    isWebSocket=false;
    websocket_buffer.clear();
    content_length=0;

}
