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
// #include "ISSL.h"
#include "events_http.hpp"
#include "resplit.h"

extern char mime_types[];
extern int mime_types_sz;
std::string datef(const time_t &__t);



UnknownBase* HTTP::Service::construct(const SERVICE_id &id, const std::string& nm,IInstance* _if)
{
    XTRY;
    return new Service(id,nm,_if);
    XPASS;
}


HTTP::Service::Service(const SERVICE_id& id, const std::string&nm, IInstance* _if):
    UnknownBase(nm),Broadcaster(_if),
    ListenerSimple(nm,id),
    iInstance(_if)
{
    m_maxPost= static_cast<size_t>(_if->getConfig()->get_int64_t("max_post", 1000000, ""));
    {
        {
            W_LOCK(mx.lk);
            // mx.docUrls=_if->getConfig()->get_stringset("doc_urls","/pics,/html,/css","");
            // mx.documentRoot=_if->getConfig()->get_string("document_root","./www","");
        }
        try {
            std::string bod;
            bod=std::string(mime_types, static_cast<unsigned int>(mime_types_sz));

            {
                W_LOCK(mx.lk);
                mx.mime_types.clear();
            }
            std::vector<std::string> v=resplit(bod,std::regex("[\r\n]"));;
            for (size_t i=0; i<v.size(); i++)
            {
                if (v[i].size())
                {
                    if (v[i][0]=='#') continue;
                }
                std::deque<std::string> dq=resplitDQ(v[i],std::regex("[\t ])"));
                if (dq.size())
                {
                    std::string typ=dq[0];
                    dq.pop_front();
                    while (dq.size())
                    {

                        {
                            W_LOCK(mx.lk);
                            mx.mime_types[dq[0]]=typ;
                        }
                        dq.pop_front();
                    }
                }
            }
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
    printf("@@@ %s\n",__FUNCTION__);
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
    return true;
}
// #include "sha1.hpp"
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
bool HTTP::Service::handleChunkedBuffer(const socketEvent::StreamRead* evt, const REF_getter<HTTP::Request>& W)
{
    auto& data=W->post_content;

    int i=0;
    while(1)
    {
        XTRY;
        auto pz=data.find("\r\n",0);


        if(pz==std::string::npos)
        {
            return true;
        }
        if(pz==0)
        {
            XTRY;
            passEvent(new httpEvent::RequestChunkingCompleted(W,W->esi,W->chunkId,evt->route));
            return true;
            XPASS;
        }


        std::string len_buf=data.substr(0,pz);
        auto chunk_size = std::stoul(len_buf, NULL, 16);
        if(chunk_size==0)
        {
            passEvent(new httpEvent::RequestChunkingCompleted(W,W->esi,W->chunkId,evt->route));
            return true;
        }

        pz=pz+2; /// + crtlf after number

        size_t payload_size=chunk_size+pz+2;
        if(data.size()>=payload_size)
        {
            auto chunk=data.substr(pz,chunk_size);
            if(data.size()==payload_size)
                data.clear();
            else
                data=data.substr(payload_size, data.size()- payload_size);
            passEvent(new httpEvent::RequestChunkReceived(W,W->esi,W->chunkId++, chunk, evt->route));
        }
        else
        {
            return true;
        }

        XPASS;
    }


}
bool HTTP::Service::on_StreamRead(const socketEvent::StreamRead* evt)
{
    MUTEX_INSPECTOR;

    REF_getter<HTTP::Request> W=getData(evt->esi.get());
    if(W->isWebSocket)
    {
        {
            W_LOCK(evt->esi->inBuffer_.lk);
            auto &data=evt->esi->inBuffer_._mx_data;
            W->websocket_buffer+=data;
            data.clear();
        }
        std::string o;
        auto res=WebSocket_getFrame((uint8_t*)W->websocket_buffer.data(),W->websocket_buffer.size(),/*out,sizeof(out),&outl,*/o);
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
            W->websocket_buffer.clear();
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

        passEvent(new httpEvent::WSTextMessage(W,o,evt->route));

        return true;
    }


    W->m_last_io_time=time(NULL);
    {
        {
            W_LOCK(evt->esi->inBuffer_.lk);
            if(!W->parse_data.done_header)
            {
                if(W->header_content.empty())
                {
                    W->header_content=std::move(evt->esi->inBuffer_._mx_data);
                    evt->esi->inBuffer_._mx_data.clear();
                }
                else
                {
                    W->header_content.append(evt->esi->inBuffer_._mx_data);
                    evt->esi->inBuffer_._mx_data.clear();
                }
            }
            else
            {
                if(W->post_content.empty())
                {
                    W->post_content=std::move(evt->esi->inBuffer_._mx_data);
                    evt->esi->inBuffer_._mx_data.clear();
                }
                else
                {
                    W->post_content.append(evt->esi->inBuffer_._mx_data);
                    evt->esi->inBuffer_._mx_data.clear();
                }
            }
        }




        if(!W->parse_data.done_header)
            W->parse(W->header_content.data(),W->header_content.size());
        if(!W->parse_data.done_header)
            return true;

        if (W->parse_data.method == HTTP::METHOD_POST && W->parse_data.post_start)
        {
            if(!W->chunked)
            {
                if(W->parse_data.header_params.TRANSFER_ENCODING.pz)
                {
                    std::string_view t=W->tosv_h(W->parse_data.header_params.TRANSFER_ENCODING);
                    if (t=="chunked")
                    {
                        logErr2("chunked");
                        W->chunked=true;
                    }
                }
            }

        }

        if(W->parse_data.header_params.CONNECTION==HTTP::CONN_UPGRADE)
        {
            {
                auto ug=W->tosv_h(W->parse_data.header_params.UPGRADE);
                if(ug=="websocket")
                {
                    auto vs=W->tosv_h(W->parse_data.header_params.Sec_WebSocket_Version);
                    if(vs!="13")
                    {
                        /// do smth for version
                    }

                    auto key=W->tosv_h(W->parse_data.header_params.Sec_WebSocket_Key);
                    std::stringstream o;
                    o <<  "HTTP/1.1 101 Switching Protocols\r\n"
                      << "Upgrade: websocket\r\n"
                      << "Connection: Upgrade\r\n";
                    auto ka=calc_key_answer(std::string(key));
                    o << "Sec-WebSocket-Accept: " << ka  << "\r\n";
                    o << "\r\n";
                    W->isWebSocket=true;
                    evt->esi->write_(o.str());
                }
            }
        }



    }
    if(W->chunked || W->isWebSocket)
    {
        if(!W->sendRequestIncomingIsSent)
        {
            W->sendRequestIncomingIsSent=true;
            passEvent(new httpEvent::RequestIncoming(W,evt->esi,evt->route));

        }
        if(W->chunked)
        {
            // logErr2("if(W->chunked)");
            handleChunkedBuffer(evt,W);
        }
        return  true;

    }

    {
        if (W->parse_data.method == HTTP::METHOD_POST)
        {
            if(W->chunked)
            {
                handleChunkedBuffer(evt,W);
            }
            std::string_view ct(W->tosv_h(W->parse_data.header_params.CONTENT_TYPE));
            if (ct.find("application/x-www-form-urlencoded", 0) != std::string_view::npos)
            {

                size_t clen = W->parse_data.header_params.CONTENT_LENGTH;
                if (clen <= 0 || clen > m_maxPost)
                {
                    return true;
                }
            }
            else if (W->parse_data.header_params.CONTENT_TYPE.pz)
            {


                //Multipart form
                const std::string_view t= W->tosv_h(W->parse_data.header_params.CONTENT_TYPE);
                auto pz = t.find("boundary=", 0);
                if (pz == std::string::npos)
                {

                    return true;
                }
                std::string bound = "--" + std::string(t.substr(pz + 9, t.length() - pz - 9));
                std::string ebound = bound + "--";
                std::string sbuf;
                {

                    {
                        W_LOCK(evt->esi->inBuffer_.lk);
                        auto &data=evt->esi->inBuffer_._mx_data;
                        auto pz=data.find(ebound);
                        if(pz!=std::string::npos)
                        {
                            sbuf=data.substr(0,pz);
                            data=data.substr(pz+ebound.size());
                        }
                        else
                            return true;
                    }

                    while (sbuf.size())
                    {


                        std::string s;
                        std::string::size_type pos=sbuf.find(bound);
                        if (pos==std::string::npos)
                        {
                            s=sbuf;
                            sbuf.clear();
                        }
                        else
                        {
                            s=sbuf.substr(0,pos);
                            sbuf=sbuf.substr(pos+bound.size(),sbuf.size()-pos-bound.size());
                        }
                        if (s.size()>1)
                        {
                            if (s[0]=='\r'&&s[1]=='\n') s=s.substr(2,s.size()-2);
                        }
                        if (s.size())
                        {

                            std::map<std::string,std::string> lparams;
                            std::string content;
                            while (1)
                            {

                                std::string sloc;
                                std::string::size_type pp=s.find("\r\n");
                                if (pp==std::string::npos)
                                {
                                    logErr2("bad multipart");
                                    return true;
                                }
                                sloc=s.substr(0,pp);
                                s=s.substr(pp+2,s.size()-pp-2);
                                if (!sloc.size())
                                {
                                    content=s.substr(0,s.size()-2);
                                    break;
                                }
                                std::string::size_type pz=sloc.find(": ");
                                if (pz==std::string::npos)
                                {
                                    logErr2("bad multipart");
                                    return  true;
                                }
                                std::string key=sloc.substr(0,pz);
                                std::string val=sloc.substr(pz+2,sloc.size()-pz-2);
                                lparams[key]=val;
                            }
                            if (lparams.count("Content-Disposition"))
                            {

                                std::string cd=lparams["Content-Disposition"];
                                std::vector<std::string> flds;
                                while (cd.size())
                                {
                                    std::string::size_type ps=cd.find("; ");
                                    if (ps==std::string::npos)
                                    {
                                        flds.push_back(cd);
                                        cd="";
                                    }
                                    else
                                    {
                                        flds.push_back(cd.substr(0,ps));
                                        cd=cd.substr(ps+2,cd.size()-2);
                                    }
                                }
                                std::string name;
                                std::string filename;
                                for (unsigned i=0; i<flds.size(); i++)
                                {
                                    std::string::size_type pzz=flds[i].find("=");
                                    if (pzz!=std::string::npos)
                                    {
                                        std::string key=flds[i].substr(0,pzz);
                                        std::string val=flds[i].substr(pzz+1,flds[i].size()-1);
                                        if (val.size()>2)
                                        {
                                            if (val[0]=='\"' && val[val.size()-1]=='\"') val=val.substr(1,val.size()-2);
                                        }
                                        if (val=="\"\"") val="";
                                        if (key=="name") name=val;
                                        if (key=="filename") filename=val;
                                    }
                                }
#ifdef PARAMS
                                W->params[name]=content;
                                if (filename.size())
                                    W->params[name+"_FILENAME"]=filename;
                                for (auto& i:lparams)
                                {
                                    std::string q = iUtils->strupper(i.first);
                                    std::string k=name + "_" + q;

                                    W->params[k] = i.second;
                                }
#endif
                            }
                        }
                    }
                }
            }
        }
        // W->parse_state.insert(2);
    }




    if(!W->sendRequestIncomingIsSent)
    {
        W->sendRequestIncomingIsSent=true;
        passEvent(new httpEvent::RequestIncoming(W,evt->esi,evt->route));
        clearData(evt->esi.get());
    }
    return  true;
}

std::string HTTP::Service::get_mime_type(const std::string& mime) const
{
    MUTEX_INSPECTOR;
    R_LOCK(mx.lk);
    auto i=mx.mime_types.find(mime);
    if (i==mx.mime_types.end()) return "";
    else return i->second;
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

/** -1 error*/ int HTTP::Service::send_other_from_disk_ext(const REF_getter<epoll_socket_info>&esi,const REF_getter<HTTP::Request>&W,const std::string & fn,const std::string& exten)
{

    MUTEX_INSPECTOR;

    W->m_last_io_time=time(NULL);

    REF_getter<HTTP::Request::_fileresponse> F=W->fileresponse;
    if(F.valid())
    {
        W->fileresponse=new HTTP::Request::_fileresponse;
        F=W->fileresponse;
    }
    F->fileName=fn;
    F->extension=exten;

    {
        if(F->io_protocol.m_open)
        {
            F->set_fd(F->io_protocol.m_open(fn.c_str(),O_RDONLY));
        }
        else
        {
            F->set_fd(::open(fn.c_str(),O_RDONLY));
        }
        if(F->get_fd()==-1)
        {
            std::string a("HTTP/1.1 404 Not Found\r\n");
            a.append("Server: nginx/1.2.6 (Ubuntu)\r\n");
            a.append("Connection: close\r\n");
            a.append("\r\n");
            esi->write_(a);
            return -1;
        }

        if(F->io_protocol.m_seek)
        {

            F->fileSize=F->io_protocol.m_seek(F->get_fd(),0,SEEK_END);

        }
        else
        {
#if defined __MACH__ || defined __FreeBSD__
            F->fileSize=lseek(F->get_fd(),0,SEEK_END);
#else
            F->fileSize=lseek64(F->get_fd(),0,SEEK_END);
#endif

        }
        if(F->fileSize==-1)
        {

            std::string a("HTTP/1.1 500 Internal Server Error\r\n");
            a.append("Server: nginx/1.2.6 (Ubuntu)\r\n");
            a.append("Connection: close\r\n");
            a.append("\r\n");
            esi->write_(a);

            return -1;
        }
    }

    F->contentLength=0;
    if (W->parse_data.header_params.RANGE.pz==0)
    {

        F->startb=0;
        F->endb=W->fileresponse->fileSize-1;
        F->written_bytes=0;
        F->hasRange=false;

    }
    else
    {

        F->startb=0;
        F->endb=F->fileSize-1;
        F->hasRange=true;
        std::string s(W->tosv_h(W->parse_data.header_params.RANGE));
        size_t n = s.find("=", 0);
        if (n != std::string::npos)
        {
            F->hasRange= true;
            std::vector <std::string> v = iUtils->splitString("-=", s.substr(n, s.size() - n));
            if (v.size() >= 1)
            {
                F->startb = atoll(v[0].c_str());
            }
            if (v.size() == 2)
            {
                F->endb  = atoll(v[1].c_str());
            }
            else
            {
                F->endb=F->fileSize-1;
            }
        }
    }

    if (F->startb > F->fileSize || F->startb < 0)
        F->startb = 0;


    if (F->endb > F->fileSize || F->endb < 0)
        F->endb = F->fileSize-1;


    if (F->endb < F->startb)
    {
        F->endb = F->fileSize-1;
        F->startb = 0;
    }

    if(F->endb==0)
        F->endb=F->fileSize-1;

    F->contentLength=F->endb-F->startb+1;


    time_t last_modified;
    {
        M_LOCK(lastModified);
        auto url=W->uri();
        if(lastModified.container.count(url))
            last_modified=lastModified.container[url];
        else
        {
            last_modified=time(NULL);
            lastModified.container[url]=last_modified;
        }

    }

    std::vector<std::string> out;
    if(F->hasRange)
        out.push_back("HTTP/1.1 206 Partial Content\r\n");
    else
        out.push_back("HTTP/1.1 200 OK\r\n");
    out.push_back("Server: nginx/1.2.6 (Ubuntu)\r\n");

    out.push_back("Date: "+datef(time(NULL))+"\r\n");

    {
        R_LOCK(mx.lk);
        auto i=mx.mime_types.find(exten);
        if(i!=mx.mime_types.end())
            out.push_back("Content-Type: "+i->second+"\r\n");
        else
            out.push_back("Content-Type: text/plain\r\n");
    }


    out.push_back("Content-length: "+std::to_string(int64_t(F->contentLength))+"\r\n");

    out.push_back("Last-Modified: "+datef(last_modified)+"\r\n");

    if(W->parse_data.header_params.CONNECTION==HTTP::CONN_KEEP_ALIVE)
        out.push_back("Connection: Keep-Alive\r\n");
    else
        out.push_back("Connection: Close\r\n");


    if(F->hasRange)
    {
        out.push_back("Content-Range: bytes "
                      +  std::to_string((int64_t)F->startb) + "-"
                      +  std::to_string((int64_t)F->endb) + "/"
                      +  std::to_string((int64_t)F->fileSize)+"\r\n");
    }
    else
    {
        out.push_back("Accept-Ranges: bytes\r\n");
    }


    out.push_back("\r\n");

    for(size_t i=0; i<out.size(); i++)
    {
        esi->write_(out[i]);
    }


    F->headerSent=true;

    if(F->io_protocol.m_seek)
    {

        F->io_protocol.m_seek(F->get_fd(),F->startb,SEEK_SET);

    }
    else
    {
#if defined __MACH__ || defined __FreeBSD__
        lseek(F->get_fd(),F->startb,SEEK_SET);
#else
        lseek64(F->get_fd(),F->startb,SEEK_SET);
#endif

    }

    return 0;
}



bool HTTP::Service::on_Disaccepted( socketEvent::Disaccepted*e)
{
    MUTEX_INSPECTOR;
    REF_getter<HTTP::Request> rq=getData(e->esi.get());
    if(rq.valid())
    {
        if(rq->isWebSocket)
        {
            passEvent(new httpEvent::WSDisaccepted(rq,e->route));
        }
    }
    clearData(e->esi.get());
    return true;
}
bool HTTP::Service::on_Disconnected( socketEvent::Disconnected*e)
{
    MUTEX_INSPECTOR;
    REF_getter<HTTP::Request> rq=getData(e->esi.get());
    if(rq.valid())
    {
        if(rq->isWebSocket)
        {
            passEvent(new httpEvent::WSDisconnected(rq,e->route));
        }
    }


    clearData(e->esi.get());

    return true;
}


REF_getter<HTTP::Request> HTTP::Service::getData(epoll_socket_info* esi)
{
    W_LOCK (esi->additions_lk);
    auto it=esi->additions_.find(ServiceEnum::HTTP);
    if(it==esi->additions_.end())
    {
        REF_getter<Refcountable> p=new HTTP::Request(esi);
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
void HTTP::Service::clearData(epoll_socket_info* esi)
{
    W_LOCK (esi->additions_lk);
    esi->additions_.erase(ServiceEnum::HTTP);

}


bool HTTP::Service::on_WSWrite(const httpEvent::WSWrite* e)
{
    // printf("on_WSWrite %s",e->msg.c_str());
    std::string msg=WebSocket_makeFrame(TEXT_FRAME,(unsigned char*)e->msg.data(),e->msg.size());
    e->r->esi->write_(msg);
    return true;
}
