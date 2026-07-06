#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "unknown.h"
#include <IInstance.h>
#include <tools_mt.h>
#include "httpService.h"
#include "IUtils.h"
#include <Events/System/Net/socketEvent.h>
#include <Events/System/Net/rpcEvent.h>
#include <Events/System/Net/httpEvent.h>
#include <version_mega.h>
#include <st_malloc.h>
#include <logging.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sstream>
// #include "ISSL.h"
#include "events_http.hpp"
#include "resplit.h"

// extern char mime_types[];
// extern int mime_types_sz;
std::string datef(const time_t &__t);

#include "sha1_1.hpp"
#include "base64.hpp"
std::string calc_key_answer(const std::string& key_ws)
{
    std::string rfc=         "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string k=key_ws+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char message_digest[20];


    websocketpp::sha1::calc(k.data(),k.size(),message_digest);
    auto key = websocketpp::base64_encode(message_digest,20);


    return key;

}
inline bool equals_case_insensitive(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); i++)
    {
        unsigned char ca = a[i];
        unsigned char cb = b[i];

        // Приводим к нижнему регистру вручную (ASCII)
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';

        if (ca != cb)
            return false;
    }
    return true;
}


UnknownBase* HTTP::Service::construct(const SERVICE_id &id, const std::string& nm,IInstance* _if)
{
    XTRY;
    return new Service(id,nm,_if);
    XPASS;
}
int on_message_begin(llhttp_t* p) 
{
    auto* r = (HTTP::Request*)p->data;
    return 0;

}
int on_url(llhttp_t* p, const char* at, size_t length) 
{
        auto* r = (HTTP::Request*)p->data;

    r->ctx->url.append(at, length);
    return 0;
}

int on_header_field(llhttp_t* p, const char* at, size_t length) 
{
    auto* r = (HTTP::Request*)p->data;
    // если было предыдущее поле — сохраняем
    if (!r->ctx->current_value.empty()) {
        r->ctx->headers[r->ctx->current_field] = r->ctx->current_value;
        r->ctx->current_field.clear();
        r->ctx->current_value.clear();
    }

    r->ctx->current_field.append(at, length);
    return 0;
}

int on_header_value(llhttp_t* p, const char* at, size_t length) 
{
    auto* r = (HTTP::Request*)p->data;
    r->ctx->current_value.append(at, length);
    return 0;
}

int on_headers_complete(llhttp_t* p) 
{
    auto* r = (HTTP::Request*)p->data;
    HTTP::Service* service=(HTTP::Service*)r->ctx->server;

    r->ctx->keepalive = llhttp_should_keep_alive(p);

    r->ctx->upgrade = p->upgrade;

    if (!r->ctx->current_field.empty()) {
        r->ctx->headers[r->ctx->current_field] = r->ctx->current_value;
    }

    r->ctx->method = llhttp_method_name((llhttp_method_t)p->method);
    r->ctx->method_int=p->method;


    if(p->flags & F_CHUNKED)
    {
        r->ctx->is_chunked=true;
        service->passEvent(new httpEvent::RequestIncoming(r->ctx,r->esi,r->currentEvent->route));
        // r->ctx=nullptr;
        return 0;

    }
    if(p->upgrade)
    {
        {
            auto ug=r->ctx->headers["Upgrade"];
            if(equals_case_insensitive(ug,"websocket"))                
            {
                auto vs=r->ctx->headers["Sec-WebSocket-Version"];
                if(vs!="13")
                {
                    /// do smth for version
                }

                auto key=r->ctx->headers["Sec-WebSocket-Key"]    ;
                std::stringstream o;
                o <<  "HTTP/1.1 101 Switching Protocols\r\n"
                    << "Upgrade: websocket\r\n"
                    << "Connection: Upgrade\r\n";
                auto ka=calc_key_answer(std::string(key));
                o << "Sec-WebSocket-Accept: " << ka  << "\r\n";
                o << "\r\n";
                r->ctx->isWebSocket=true;
                r->esi->write_(o.str());
            }
        }
        return 1;
    }


    return 0;
}

int on_body(llhttp_t* p, const char* at, size_t length) 
{
    auto* r = (HTTP::Request*)p->data;
    HTTP::Service* service=(HTTP::Service*)r->ctx->server;

    if(r->ctx->is_chunked)
    {
        if(!r->ctx->body.valid())
        {
            r->ctx->body=new refbuffer();
        }   
        r->ctx->body->container.append(at,length);
        return 0;
    }
    else
    {
        if(!r->ctx->body.valid())
        {
            r->ctx->body=new refbuffer();
        }   
        r->ctx->body->container.append(at,length);
        return 0;
    }

    return 0;
}

int on_message_complete(llhttp_t* p) {
    auto* r = (HTTP::Request*)p->data;

    HTTP::Service* service=(HTTP::Service*)r->ctx->server;

    if(r->ctx->is_chunked)
    {
        auto c=r->ctx;
        service->passEvent(new httpEvent::RequestChunkingCompleted(r->ctx,r->esi,r->ctx->chunkId, r->currentEvent->route));
        r->ctx=new HttpContext(c->server,c->esi); 
        return 0;
    }
    if(r->ctx->method_int==HTTP_GET)
    {
        auto c=r->ctx;
        service->passEvent(new httpEvent::RequestIncoming(c,r->esi,r->currentEvent->route));
        r->ctx=new HttpContext(c->server,c->esi); 

    }
    else if(r->ctx->method_int==HTTP_POST)
    {
        auto c=r->ctx;
        service->passEvent(new httpEvent::RequestIncoming(r->ctx,r->esi,r->currentEvent->route));
        r->ctx=new HttpContext(c->server,c->esi); 
    }

    return 0;
}
int on_chunk_header(llhttp_t* p) {
    auto* r = (HTTP::Request*)p->data;
    r->ctx->body=nullptr;
    r->ctx->chunk_size=p->content_length;
    return 0;
}
int on_chunk_complete(llhttp_t* p) {

    auto* r = (HTTP::Request*)p->data;
    HTTP::Service* service=(HTTP::Service*)r->ctx->server;
    if(!r->ctx->body.valid())
    {
        // logErr2("chunk body is not valid %d",r->ctx->chunk_size);
        return 0;
    }
    if(r->ctx->body->container.size()!=r->ctx->chunk_size)
        throw CommonError("chunk size mismatch");
    service->passEvent(new httpEvent::RequestChunkReceived(r->ctx,r->esi,r->ctx->chunkId++, r->ctx->body, r->currentEvent->route));
    r->ctx->body=nullptr;
    r->ctx->chunk_size=0;

    return 0;
}

HTTP::Service::Service(const SERVICE_id& id, const std::string&nm, IInstance* _if):
    UnknownBase(nm),Broadcaster(_if),
    ListenerSimple(nm,id),
    iInstance(_if)
{
    llhttp_settings_init(&settings);
    settings.on_message_begin = on_message_begin;
    settings.on_url = on_url;
    settings.on_header_field = on_header_field;
    settings.on_header_value = on_header_value;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
    settings.on_chunk_header = on_chunk_header;
    settings.on_chunk_complete = on_chunk_complete;

    m_maxPost= static_cast<size_t>(_if->getConfig()->get_int64_t("max_post", 1000000, ""));
    {
        {
            W_LOCK(mx.lk);
            // mx.docUrls=_if->getConfig()->get_stringset("doc_urls","/pics,/html,/css","");
            // mx.documentRoot=_if->getConfig()->get_string("document_root","./www","");
        }
        try {
        }
        catch(...) {}
    }
}

bool HTTP::Service::on_Connected(const socketEvent::Connected*)
{
    MUTEX_INSPECTOR;

    return true;

}
bool HTTP::Service::on_NotifyBindAddress(const socketEvent::NotifyBindAddress*e)
{
    MUTEX_INSPECTOR;

    W_LOCK(mx.lk);
    mx.bind_addrs.insert(e->addr);
    return true;
}


bool HTTP::Service::on_DoListen(const httpEvent::DoListen* e)
{
    MUTEX_INSPECTOR;

    SOCKET_id newid=iUtils->getNewSocketId();
    msockaddr_in sa=e->addr;
    DBG(logErr2("on_DoListen %s",e->route.dump().c_str()));
    sendEvent(socketListener,new socketEvent::AddToListenTCP(newid,sa,"HTTP",false,e->secure,e->route));

    return true;
}

bool HTTP::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;
    socketListener=dynamic_cast<ListenerBase*>(iInstance->getServiceOrCreate(ServiceEnum::Socket));
    if(!socketListener)
        throw CommonError("if(!socketListener)");
    return true;
}
bool HTTP::Service::on_GetBindPortsREQ(const httpEvent::GetBindPortsREQ *e)
{
    MUTEX_INSPECTOR;
    R_LOCK(mx.lk);
    passEvent(new httpEvent::GetBindPortsRSP(mx.bind_addrs,poppedFrontRoute(e->route)));
    return true;
}

bool HTTP::Service::handleEvent(const REF_getter<Event::Base>& evt)
{
    MUTEX_INSPECTOR;
    auto &ID=evt->id;

    switch(ID)
    {
    case httpEventEnum::WSWrite:
        return on_WSWrite((httpEvent::WSWrite*)evt.get());

    case socketEventEnum::Disaccepted:
        return on_Disaccepted((socketEvent::Disaccepted*)evt.get());
    case socketEventEnum::Disconnected:
        return on_Disconnected((socketEvent::Disconnected*)evt.get());

    case socketEventEnum::Accepted:
        return on_Accepted((const socketEvent::Accepted*)evt.get());
    case socketEventEnum::StreamRead:
        return on_StreamRead((const socketEvent::StreamRead*)evt.get());
    case socketEventEnum::Connected:
        return on_Connected((const socketEvent::Connected*)evt.get());
    case socketEventEnum::NotifyBindAddress:
        return on_NotifyBindAddress((const socketEvent::NotifyBindAddress*)evt.get());
    case socketEventEnum::NotifyOutBufferEmpty:
        return on_NotifyOutBufferEmpty((const socketEvent::NotifyOutBufferEmpty*)evt.get());
    case httpEventEnum::DoListen:
        return(this->on_DoListen((const httpEvent::DoListen*)evt.get()));
    case httpEventEnum::GetBindPortsREQ:
        return(this->on_GetBindPortsREQ((const httpEvent::GetBindPortsREQ*)evt.get()));
    case systemEventEnum::startService:
        return on_startService((const systemEvent::startService*)evt.get());
    case rpcEventEnum::IncomingOnAcceptor:
    {
        MUTEX_INSPECTOR;
        rpcEvent::IncomingOnAcceptor *E=(rpcEvent::IncomingOnAcceptor *)evt.get();
        auto IDA=E->e->id;
        if(httpEventEnum::GetBindPortsREQ==IDA)
        {
            MUTEX_INSPECTOR;
            const httpEvent::GetBindPortsREQ *e=(const httpEvent::GetBindPortsREQ *)E->e.get();
            R_LOCK(mx.lk);
            passEvent(new httpEvent::GetBindPortsRSP(mx.bind_addrs,poppedFrontRoute(e->route)));
            return true;
        }
        return false;
    }

    }

    return false;
}

bool HTTP::Service::on_Accepted(const socketEvent::Accepted* evt)
{
    MUTEX_INSPECTOR;
    REF_getter<Refcountable> p=new HTTP::Request(evt->esi, settings,this);
    evt->esi->additions_.insert(std::make_pair(ServiceEnum::HTTP,p));

    return true;
}
// #include "sha1.hpp"
HTTP::WebSocketFrameType HTTP::Service::WebSocket_getFrame(unsigned char* in_buffer, int in_length, /*unsigned char* out_buffer, int out_size, int* out_length,*/ std::string &o)
{
    if(in_length < 2) return INCOMPLETE_FRAME;

    unsigned char msg_opcode = in_buffer[0] & 0x0F;
    unsigned char msg_fin = (in_buffer[0] >> 7) & 0x01;
    unsigned char msg_masked = (in_buffer[1] >> 7) & 0x01;

    // *** message decoding

    int payload_length = 0;
    int pos = 2;
    int length_field = in_buffer[1] & (~0x80);
    unsigned int mask = 0;

    //printf("IN:"); for(int i=0; i<20; i++) printf("%02x ",buffer[i]); printf("\n");

    if(length_field <= 125) {
        payload_length = length_field;
    }
    else if(length_field == 126) { //msglen is 16bit!
        //payload_length = in_buffer[2] + (in_buffer[3]<<8);
        payload_length = (
                             (in_buffer[2] << 8) |
                             (in_buffer[3])
                         );
        pos += 2;
    }
    else if(length_field == 127) { //msglen is 64bit!
        payload_length = (
                             (uint64_t(in_buffer[2]) << 56) |
                             (uint64_t(in_buffer[3]) << 48) |
                             (uint64_t(in_buffer[4]) << 40) |
                             (uint64_t(in_buffer[5]) << 32) |
                             (uint64_t(in_buffer[6]) << 24) |
                             (uint64_t(in_buffer[7]) << 16) |
                             (uint64_t(in_buffer[8]) << 8) |
                             (uint64_t(in_buffer[9]))
                         );
        pos += 8;
    }

    //printf("PAYLOAD_LEN: %08x\n", payload_length);
    if(in_length < payload_length+pos) {
        return HTTP::INCOMPLETE_FRAME;
    }

    if(msg_masked) {
        mask = *((unsigned int*)(in_buffer+pos));
        //printf("MASK: %08x\n", mask);
        pos += 4;

        // unmask data:
        unsigned char* c = in_buffer+pos;
        for(int i=0; i<payload_length; i++) {
            c[i] = c[i] ^ ((unsigned char*)(&mask))[i%4];
        }
    }

    // if(payload_length > out_size) {
    //TODO: if output buffer is too small -- ERROR or resize(free and allocate bigger one) the buffer ?
    // }
    o+=std::string((char*)(in_buffer+pos),payload_length);
    // memcpy((void*)out_buffer, (void*)(in_buffer+pos), payload_length);
    // out_buffer[payload_length] = 0;
    // *out_length = payload_length+1;

    //printf("TEXT: %s\n", out_buffer);

    if(msg_opcode == 0x0) return (msg_fin)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME; // continuation frame ?
    if(msg_opcode == 0x1) return (msg_fin)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME;
    if(msg_opcode == 0x2) return (msg_fin)?BINARY_FRAME:INCOMPLETE_BINARY_FRAME;
    if(msg_opcode == 0x9) return PING_FRAME;
    if(msg_opcode == 0xA) return PONG_FRAME;

    return ERROR_FRAME;
}
std::string HTTP::Service::WebSocket_makeFrame(WebSocketFrameType frame_type, unsigned char* msg, int msg_length/*, unsigned char* buffer, int buffer_size*/)
{
    unsigned char* buffer=(unsigned char*)malloc(msg_length+1000);

    int pos = 0;
    int size = msg_length;
    buffer[pos++] = (unsigned char)frame_type; // text frame

    if(size <= 125) {
        buffer[pos++] = size;
    }
    else if(size <= 65535) {
        buffer[pos++] = 126; //16 bit length follows

        buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
        buffer[pos++] = size & 0xFF;
    }
    else { // >2^16-1 (65535)
        buffer[pos++] = 127; //64 bit length follows

        // write 8 bytes length (significant first)

        // since msg_length is int it can be no longer than 4 bytes = 2^32-1
        // padd zeroes for the first 4 bytes
        for(int i=3; i>=0; i--) {
            buffer[pos++] = 0;
        }
        // write the actual 32bit msg_length in the next 4 bytes
        for(int i=3; i>=0; i--) {
            buffer[pos++] = ((size >> 8*i) & 0xFF);
        }
    }
    memcpy((void*)(buffer+pos), msg, size);
    std::string ret((char*)buffer,size+pos);
    free(buffer);
    return ret;

    // return (size+pos);
}
#include "llhttp/llhttp.h"

bool HTTP::Service::on_StreamRead(const socketEvent::StreamRead* evt)
{
    MUTEX_INSPECTOR;

    REF_getter<HTTP::Request> W=getData(evt->esi.get());
    if(!W.valid())
        throw CommonError("if(!W.valid())");
    if(!W->ctx.valid())
        throw CommonError("if(!W->ctx.valid())");
    if(W->ctx->isWebSocket)
    {
        {
            W_LOCK(evt->esi->inBuffer_.lk);
            auto &data=evt->esi->inBuffer_._mx_data;
            W->ctx->websocket_buffer+=data;
            data.clear();
        }
        std::string o;
        auto res=WebSocket_getFrame((uint8_t*)W->ctx->websocket_buffer.data(),W->ctx->websocket_buffer.size(),/*out,sizeof(out),&outl,*/o);
        switch(res)
        {
        case ERROR_FRAME:
            logErr2("case ERROR_FRAME:");
            return true;
            break;

        case INCOMPLETE_FRAME:
            logErr2("INCOMPLETE_FRAME");
            break;
        case OPENING_FRAME:
            logErr2("OPENING_FRAME");
            break;
        case CLOSING_FRAME:
            logErr2("CLOSING_FRAME");
            break;
        case INCOMPLETE_TEXT_FRAME:
            logErr2("INCOMPLETE_TEXT_FRAME");
            break;
        case INCOMPLETE_BINARY_FRAME:
            logErr2("INCOMPLETE_BINARY_FRAME");
            break;
        case TEXT_FRAME:
            W->ctx->websocket_buffer.clear();
            // logErr2("TEXT_FRAME");
            break;
        case BINARY_FRAME:
            logErr2("BINARY_FRAME");
            break;
        case PING_FRAME:
            logErr2("PING_FRAME");
            break;
        case PONG_FRAME:
            logErr2("PONG_FRAME");
            break;
        };

        passEvent(new httpEvent::WSTextMessage(W->ctx,o,evt->route));

        return true;
    }


    // W->m_last_io_time=time(NULL);
    std::string buf;
    {
        W_LOCK(evt->esi->inBuffer_.lk);
        buf=std::move(evt->esi->inBuffer_._mx_data);
        evt->esi->inBuffer_._mx_data.clear();
    }
    // logErr2("on_StreamRead %s bytes",buf.c_str());
    // logErr2("before llhttp_execute");
    W->currentEvent=evt;
    llhttp_errno_t err = llhttp_execute(&W->parser, buf.data(), buf.size());
    // logErr2("after llhttp_execute");
    if (err != HPE_OK) 
    {
        throw CommonError("Parse error: %s",llhttp_errno_name(err));
    }
    return true;
}
bool HTTP::Service::on_NotifyOutBufferEmpty(const socketEvent::NotifyOutBufferEmpty* e)
{
    MUTEX_INSPECTOR;

    return true;
    S_LOG("on_NotifyOutBufferEmpty");

    REF_getter<HTTP::Request> W=getData(e->esi.get());
    if(!W.valid())
    {

        e->esi->close("HTTP::Request not exists");
        return true;
    }
    #ifdef KALL
    REF_getter<HTTP::Request::_fileresponse> F=W->fileresponse;
    if(!F.valid())
        return true;
    if(F->fileSize==0)
    {
        e->esi->close("HTTPService: on_NotifyOutBufferEmpty: F->fileSize==0 @1");
        return true;
    }
    if(F->fileSize==-1)
    {
        e->esi->close("HTTPService: on_NotifyOutBufferEmpty: F->fileSize==-1 @2");
        return true;
    }
    if(F->contentLength<=F->written_bytes)
    {
        e->esi->close("HTTPService: on_NotifyOutBufferEmpty: F->contentLength>=F->written_bytes");
        return true;
    }
    int64_t offset=F->startb+F->written_bytes;
    {
        size_t bufsize=0x10000;
        st_malloc buf(bufsize);
        {

            int64_t readSize=bufsize;
            if(F->written_bytes+readSize > F->contentLength)
            {
                readSize=F->contentLength-F->written_bytes;
            }

            if(offset+readSize>F->fileSize)
            {
                readSize=F->fileSize-offset;
            }

            int64_t res=-1;
            if(W->fileresponse->io_protocol.m_read)
            {

                res=F->io_protocol.m_read(F->get_fd(),(unsigned char*)buf.buf,(size_t)readSize);
            }
            else
            {

                res=read(F->get_fd(),(char*)buf.buf,(size_t)readSize);
            }

            if(res==-1)
            {

                e->esi->close("HTTPService: on_NotifyOutBufferEmpty: read returns -1");
                return true;
            }

            if(res==0)
            {

                e->esi->close("HTTPService: on_NotifyOutBufferEmpty: read returns 0 (EOF)");
                return true;
            }

            e->esi->write_buf((char*)buf.buf,res);
            F->written_bytes+=res;
            return true;
        }
    }
  #endif
    return true;
}

void registerHTTPModule(const char* pn)
{
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::HTTP,"HTTP",getEvents_http());
    }
    else
    {
        iUtils->registerService(ServiceEnum::HTTP,HTTP::Service::construct,"HTTP");
        regEvents_http();
    }
}

std::string datef(const time_t &__t)
{
    MUTEX_INSPECTOR;
    time_t t=__t-100000;
    static char const * wkday[]        = {"Sun","Mon","Tue","Wed", "Thu","Fri", "Sat"};
    static char const* month[]        = {"Jan", "Feb", "Mar", "Apr"
                                         , "May", "Jun", "Jul", "Aug"
                                         , "Sep", "Oct", "Nov", "Dec"
                                        };

    char outstr[200];
    struct tm tt;
    localtime_r(&t,&tt);
    snprintf(outstr,sizeof(outstr),"%s, %02d %s %d %02d:%02d:%02d GMT",
             wkday[tt.tm_wday%7],tt.tm_mday,month[tt.tm_mon%12],tt.tm_year+1900,tt.tm_hour,tt.tm_min,tt.tm_sec);
    return outstr;
}


bool HTTP::Service::on_Disaccepted( socketEvent::Disaccepted*e)
{
    MUTEX_INSPECTOR;
    REF_getter<HTTP::Request> rq=getData(e->esi.get());
    if(rq.valid())
    {
        if(rq->ctx->isWebSocket)
        {
            passEvent(new httpEvent::WSDisaccepted(rq,e->route));
        }
    }
    // clearData(e->esi.get());
    return true;
}
bool HTTP::Service::on_Disconnected( socketEvent::Disconnected*e)
{
    MUTEX_INSPECTOR;
    REF_getter<HTTP::Request> rq=getData(e->esi.get());
    if(rq.valid())
    {
        if(rq->ctx->isWebSocket)
        {
            passEvent(new httpEvent::WSDisconnected(rq->ctx,e->route));
        }
    }


    // clearData(e->esi.get());

    return true;
}


REF_getter<HTTP::Request> HTTP::Service::getData(epoll_socket_info* esi)
{
    W_LOCK (esi->additions_lk);
    auto it=esi->additions_.find(ServiceEnum::HTTP);
    if(it==esi->additions_.end())
    {
        // throw CommonError("HTTP::Service::getData: HTTP::Request not found for esi %p",esi);
        // logErr2("HTTP::Service::getData: create new HTTP::Request for esi %p",esi);
        REF_getter<Refcountable> p=new HTTP::Request(esi, settings,this);
        esi->additions_.insert(std::make_pair(ServiceEnum::HTTP,p));
        it=esi->additions_.find(ServiceEnum::HTTP);
    }
    auto ret=dynamic_cast<HTTP::Request*>(it->second.get());
    if(ret==NULL)
        throw CommonError("if(ret==NULL)");
    return ret;

}
void HTTP::Service::setData(epoll_socket_info* esi, const REF_getter<HTTP::Request> & p)
{
    W_LOCK (esi->additions_lk);
    esi->additions_.insert(std::make_pair(ServiceEnum::HTTP,p.get()));

}

bool HTTP::Service::on_WSWrite(const httpEvent::WSWrite* e)
{
    // printf("on_WSWrite %s",e->msg.c_str());
    std::string msg=WebSocket_makeFrame(TEXT_FRAME,(unsigned char*)e->msg.data(),e->msg.size());
    e->r->esi->write_(msg);
    return true;
}
