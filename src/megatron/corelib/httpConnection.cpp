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
    if(request->parse_data.header_params.CONNECTION==HTTP::CONN_KEEP_ALIVE && !is_chunked)
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
    return ret.str();
}

inline int ascii_tolower(int c) {
    return (c >= 'A' && c <= 'Z') ? (c + 32) : c;
}
// Самые частые заголовки для оптимизации сравнения
typedef enum {
    HOST,
    USER_AGENT,
    ACCEPT,
    ACCEPT_LANGUAGE,
    ACCEPT_ENCODING,
    CONNECTION,
    CONTENT_TYPE,
    CONTENT_LENGTH,
    COOKIE,
    REFERER,
    RANGE,
    AUTHORIZATION,
    TRANSFER_ENCODING,
    UPGRADE,
    Sec_WebSocket_Key,
    Sec_WebSocket_Version,
    EXPECT,

    UNKNOWN_HEADER
} common_header_t;

constexpr std::string_view _HOST="Host";
constexpr std::string_view _USER_AGENT="User-Agent";

constexpr std::string_view _ACCEPT="Accept";
constexpr std::string_view _ACCEPT_LANGUAGE="Accept-Language";
constexpr std::string_view _ACCEPT_ENCODING="Accept-Encoding";
constexpr std::string_view _CONNECTION="Connection";
constexpr std::string_view _CONTENT_TYPE="Content-Type";
constexpr std::string_view _CONTENT_LENGTH="Content-Length";
constexpr std::string_view _COOKIE="Cookie";
constexpr std::string_view _REFERER="Referer";
constexpr std::string_view _AUTHORIZATION="Authorization";
constexpr std::string_view _RANGE="Range";
constexpr std::string_view _Transfer_Encoding="Transfer-Encoding";
constexpr std::string_view _UPGRADE="Upgrade";
constexpr std::string_view _Sec_WebSocket_Key="Sec-WebSocket-Key";
constexpr std::string_view _Sec_WebSocket_Version="Sec-WebSocket-Version";
constexpr std::string_view _Keep_Alive="Keep-Alive";
constexpr std::string_view _keep_alive="keep-alive";
constexpr std::string_view _Close="Close";
constexpr std::string_view _close="close";
constexpr std::string_view _Expect="Expect";


common_header_t get_common_header_type(const std::string_view &t) {
    // Быстрая проверка самых частых заголовков
    switch (t[0]) {
    case 'E':
        if (t==_Expect) return EXPECT;
        break;
    case 'H':
        if (t==_HOST) return HOST;
        break;
    case 'U':
        if (t==_USER_AGENT) return USER_AGENT;
        if (t==_UPGRADE) return UPGRADE;
        break;
    case 'A':
        if (t==_ACCEPT) return ACCEPT;
        if (t==_ACCEPT_LANGUAGE) return ACCEPT_LANGUAGE;
        if (t==_ACCEPT_ENCODING) return ACCEPT_ENCODING;
        if (t==_AUTHORIZATION) return AUTHORIZATION;
        break;
    case 'C':
        if (t==_CONNECTION) return CONNECTION;
        if (t==_CONTENT_TYPE) return CONTENT_TYPE;
        if (t==_CONTENT_LENGTH) return CONTENT_LENGTH;
        if (t==_COOKIE) return COOKIE;
        break;
    case 'R':
        if (t==_REFERER) return REFERER;
        if (t==_RANGE) return RANGE;
        break;
    case 'T':
        if (t==_Transfer_Encoding) return TRANSFER_ENCODING;
        break;
    case 'S':
        if (t==_Sec_WebSocket_Key) return Sec_WebSocket_Key;
        if (t==_Sec_WebSocket_Version) return Sec_WebSocket_Version;


    }
    return UNKNOWN_HEADER;
}
void HTTP::http_header_parse_data::dump(const char* req, int reqsize)
{
    logErr2("1st line: %d",line0tk_idx);
    for(int i=0; i< line0tk_idx; i++)
    {
        for(int j=0; j<line0tk[i].len; j++)
        {
            putchar(req[line0tk[i].pz+j]);
        }
        printf("\n");
    }
}

void print_token(const char* p, const HTTP::token& t)
{
    for(int i=0; i<t.len; i++)
    {
        putchar(p[t.pz+i]);
    }
}
#include <iostream>
#include "sv.h"
void HTTP::Request::parse(const char *req, int reqsize)
{
    while(parse_data.cur_pos<reqsize && ! parse_data.done_header)
    {
        if(parse_data.cur_line==0)
        {
            if(isspace(parse_data.last_char) && !isspace(req[parse_data.cur_pos]))
            {
                if(parse_data.line0tk_idx>=LINE0TKSIZE)
                    throw CommonError("if(line0tk_idx>=LINE0TKSIZE)");
                parse_data.line0tk[parse_data.line0tk_idx++].pz=parse_data.cur_pos;
            }
            if(!isspace(parse_data.last_char) && isspace(req[parse_data.cur_pos]))
            {
                if(parse_data.line0tk_idx==0)
                    throw CommonError("if(line0tk_idx==0)");
                parse_data.line0tk[parse_data.line0tk_idx-1].len=parse_data.cur_pos-parse_data.line0tk[parse_data.line0tk_idx-1].pz;

            }
            if(parse_data.line0tk_idx==3)
            {
                const char *m=&req[parse_data.line0tk[0].pz];
                if(!memcmp(m,"GET",3)) parse_data.method=HTTP::METHOD_GET;
                else if(!memcmp(m,"POST",4)) parse_data.method=HTTP::METHOD_POST;
                else if(!memcmp(m,"PUT",3)) parse_data.method=HTTP::METHOD_PUT;
                else if(!memcmp(m,"DELETE",6)) parse_data.method=HTTP::METHOD_DELETE;
                else throw CommonError("wrong method");
                parse_data.uri=parse_data.line0tk[1];
                auto p=tosv_h(parse_data.uri);
                parse_data.http_version=parse_data.line0tk[2];
            }
        }
        if(req[parse_data.cur_pos]=='\n' && parse_data.last_char=='\r')
        {
            if(parse_data.cur_line_size==0)
            {
                /// end header;
                parse_data.done_header=true;
                parse_data.post_start=parse_data.cur_pos+1;

                // logErr2("parse_data.post_start %d headerconten sz %d",parse_data.post_start,header_content.size());
                if(parse_data.post_start!=header_content.size())
                {
                    post_content+=header_content.substr(parse_data.post_start);
                }
                // logErr2("post content added %d",post_content.size());
                break;
            }
            token H= {parse_data.cur_line_start,parse_data.cur_line_size};

            if(parse_data.cur_line>0)
            {
                std::string_view line(&req[parse_data.cur_line_start],parse_data.cur_line_size);
                auto pz=line.find(':');
                if(pz==std::string_view::npos)
                    throw CommonError("if(pz==std::string_view::npos)");
                {
                    std::string_view name=line.substr(0,pz);
                    // const char *p=name.data();
                    // auto sz=name.size();
                    // for(int i=0;i<sz;i++)
                    // {
                    //     char *c=(char*)&p[i];
                    //     if(*c>='A' || *c<='Z')
                    //     *c+=0x20;
                    //     // if(name.data()[i])
                    // }
                    pz+=2;

                    token value= {H.pz+pz,H.len-pz};
                    auto ht=get_common_header_type(name);
                    switch(ht)
                    {
                    case HOST:
                        parse_data.header_params.HOST=value;
                        break;
                    case EXPECT:
                        parse_data.header_params.EXPECT=value;
                        break;
                    case USER_AGENT:
                        parse_data.header_params.USER_AGENT=value;
                        break;
                    case ACCEPT:
                        parse_data.header_params.ACCEPT=value;
                        break;
                    case ACCEPT_LANGUAGE:
                        parse_data.header_params.ACCEPT_LANGUAGE=value;
                        break;
                    case ACCEPT_ENCODING:
                        parse_data.header_params.ACCEPT_ENCODING=value;
                        break;
                    case CONNECTION:
                    {
                        std::string_view v(&req[value.pz],value.len);
                        if(v==_Keep_Alive)
                        {
                            parse_data.header_params.CONNECTION=HTTP::CONN_KEEP_ALIVE;
                        }
                        else if(v==_keep_alive)
                        {
                            parse_data.header_params.CONNECTION=HTTP::CONN_KEEP_ALIVE;
                        }
                        else if(v==_UPGRADE)
                        {
                            parse_data.header_params.CONNECTION=HTTP::CONN_UPGRADE;
                        }
                        else parse_data.header_params.CONNECTION=HTTP::CONN_CLOSE;
                        // else throw CommonError("Unknown connection type %s",std::string(v).c_str());

                    }
                    break;

                    case CONTENT_TYPE:
                        parse_data.header_params.CONTENT_TYPE=value;
                        break;
                    case UPGRADE:
                        parse_data.header_params.UPGRADE=value;
                        break;
                    case CONTENT_LENGTH:
                    {
                        int v;
                        auto result = std::from_chars(&req[value.pz], req+value.pz+value.len, v);

                        if (result.ec == std::errc()) {
                            parse_data.header_params.CONTENT_LENGTH=v;
                        } else {
                            throw CommonError("!if (result.ec == std::errc())");
                        }
                    }
                    break;
                    case COOKIE:
                        parse_data.header_params.COOKIE=value;
                        break;
                    case REFERER:
                        parse_data.header_params.REFERER=value;
                        break;
                    case RANGE:
                        parse_data.header_params.RANGE=value;
                        break;
                    case AUTHORIZATION:
                        parse_data.header_params.AUTHORIZATION=value;
                        break;
                    case TRANSFER_ENCODING:
                        parse_data.header_params.TRANSFER_ENCODING=value;
                        break;
                    case Sec_WebSocket_Key:
                        parse_data.header_params.Sec_WebSocket_Key=value;
                        break;
                    case Sec_WebSocket_Version:
                        parse_data.header_params.Sec_WebSocket_Version=value;
                        break;

                    case UNKNOWN_HEADER:
                        std::cout<< "UNKNOWN_HEADER " << name << std::endl;
                        std::cout.flush();
                        throw CommonError("case UNKNOWN_HEADER:");
                    default:
                        throw CommonError("default UNKNOWN_HEADER:");
                    }




                }
            }
            // header_lines[header_lines_idx]={cur_line_start,cur_line_size};
            // header_lines_idx++;
            parse_data.cur_line++;
            parse_data.cur_line_size=0;
            parse_data.cur_line_start=parse_data.cur_pos+1;
        }
        if(req[parse_data.cur_pos]!='\r' && req[parse_data.cur_pos]!='\n')
            parse_data.cur_line_size++;
        // if(cur_pos>1)
        parse_data.last_char=req[parse_data.cur_pos];
        parse_data.cur_pos++;

    }
}
HTTP::Request::Request(const REF_getter<epoll_socket_info>& _esi)
    :
    m_last_io_time(time(NULL)),
    fileresponse(NULL)
    ,
    sendRequestIncomingIsSent(false),
    esi(_esi)
{
    memset(& parse_data,0,sizeof(parse_data));
    parse_data.last_char=' ';
}
void HTTP::Response::make_response(const std::string& str)
{

    // logErr2("@@ %s",__PRETTY_FUNCTION__);
    if(is_chunked)
        throw CommonError("if(is_chunked)");



    content=str;
    auto s=build_html_response();

    if( request->parse_data.header_params.CONNECTION==HTTP::CONN_KEEP_ALIVE)
    {
        // logErr2("request->esi->write_(s);");
        request->esi->write_(s);
    }
    else
    {
        // logErr2("request->esi->write_and_close(s);");
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
    // int socketId=CONTAINER(request->esi->id_);
    if(!is_chunked)
        throw CommonError("if(!is_chunked)");
    if(content.size())
        throw CommonError("if(content.size())");
    {
        // if(content.size())
        {

            // auto cc=std::move(content);
            // content.clear();
            char buf[100];
            snprintf(buf,sizeof(buf),"0\r\n\r\n");
            // auto s=buf;
            // logErr2("end:chunked:ka%d CS sid(%d) %s",request->connection==HTTP::Request::KEEPALIVE,socketId,dline(s).c_str());
            // request->connection==HTTP::Request::KEEPALIVE &&
            if( request->parse_data.header_params.CONNECTION==HTTP::CONN_KEEP_ALIVE)
            {
                request->esi->write_(buf);
            }
            else
            {
                request->esi->write_and_close(buf);
            }

            // content.clear();
        }
        // else
        // {
        //     std::string s="0\r\n\r\n";
        //     // logErr2("end:chunked:ka%d !content.size sid(%d) %s",request->connection==HTTP::Request::KEEPALIVE, socketId,dline(s).c_str());
        //     // printf("end:chunked:ka%d %s",request->connection==HTTP::Request::KEEPALIVE,s.c_str());
        //     if(request->connection==HTTP::Request::KEEPALIVE && !is_chunked)
        //     {
        //         logErr2("write_ %s",s.c_str());
        //         request->esi->write_(s);
        //     }
        //     else
        //     {
        //         logErr2("write_and_close %s",s.c_str());
        //         request->esi->write_and_close(s);
        //     }

        // }
        // logErr2("content after clear '%s'",content.c_str());
        // std::string s="0\r\n\r\n";
        // request->esi->write_(s);



    }
    // else
    // {
    //     // logErr2("w cl content %s",content.c_str());

    //     std::string out = build_html_response();
    //     // logErr2("end:content:ka%d %s",request->connection==HTTP::Request::CONN::KEEPALIVE, out.c_str());
    //     if(request->connection==HTTP::Request::CONN::KEEPALIVE && !is_chunked)
    //     {
    //         request->esi->write_(out);
    //     }
    //     else
    //     {
    //         request->esi->write_and_close(out);

    //     }

    //     content.clear();
    //     // logErr2("content after clear %s",content.c_str());
    // }

}



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
