#include "webSocketClientService.h"
#include <stdarg.h>

#include "Events/System/Net/socketEvent.h"
#include "events_webSocketClient.hpp"
#include "url.hpp"
#include <nlohmann/json.hpp>
webSocketClient::Service::Service(const SERVICE_id& id, const std::string& nm, IInstance *ifa):
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),Broadcaster(ifa)

{


}

webSocketClient::Service::~Service()
{
}
bool webSocketClient::Service::on_startService(const systemEvent::startService* e)
{
    if(!e) throw CommonError("!e");
    return true;
}

void registerWebSocketClientService(const char* pn)
{
    XTRY;
    if(pn)
    {

        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::WebSocketClient,"WebSocketClient",getEvents_webSocketClient());
    }
    else
    {
        iUtils->registerService(ServiceEnum::WebSocketClient,webSocketClient::Service::construct,"WebSocketClient");
        regEvents_webSocketClient();
    }
    XPASS;
}


bool webSocketClient::Service::Connect(const webSocketClientEvent::Connect* e)
{
    REF_getter<state> st=new state;

    states.insert({e->socketId,st});
    st->url=Url(e->url);

    auto host=st->url.host();
    auto scheme=st->url.scheme();
    auto port=st->url.port();
    auto path=st->url.path();

    nlohmann::json j;
    j["host"]=host;
    j["scheme"]=scheme;
    j["port"]=port;
    j["path"]=path;
    SECURE s;
    std::string sc;
    if(!scheme.size())
    {
        sc="ws";
    }
    else sc=scheme;

    int iport=80;

    if(sc=="ws")
    {
        s.use_ssl=false;
        iport=80;
    }
    else if(sc=="wss")
    {
        s.use_ssl=true;
        iport=443;
    }
    else throw CommonError("invalid scheme");


    if(port.size())
        iport=atoi(port.c_str());


    msockaddr_in sa;
    sa.init(host,iport);
    sendEvent(ServiceEnum::Socket, new socketEvent::AddToConnectTCP(e->socketId,sa,e->socketDescription,s,e->route));
    return true;
}
bool webSocketClient::Service::Send(const webSocketClientEvent::Send* e)
{
    std::vector<unsigned char> frame;
    frame.push_back(0x81); // FIN + текстовый фрейм

    if (e->msg.size() <= 125) {
        frame.push_back(static_cast<unsigned char>(e->msg.size()));
    } else if (e->msg.size() <= 65535) {
        frame.push_back(126);
        frame.push_back((e->msg.size() >> 8) & 0xFF);
        frame.push_back(e->msg.size() & 0xFF);
    } else {
        throw std::runtime_error("Сообщение слишком большое");
    }

    for (char c : e->msg) {
        frame.push_back(c);
    }
    e->esi->write_({(char*)frame.data(), frame.size()});

    return true;
}
bool webSocketClient::Service::ConnectFailed(const socketEvent::ConnectFailed* e)
{
    passEvent(new webSocketClientEvent::ConnectFailed(e->esi,e->route));
    return true;
}
bool webSocketClient::Service::NotifyOutBufferEmpty(const socketEvent::NotifyOutBufferEmpty*)
{
    return true;
}


bool webSocketClient::Service::Connected(socketEvent::Connected* e)
{
    auto it=states.find(e->esi->id_);
    if(it==states.end())
        throw CommonError("if(it==states.end())");


    // states.insert({e->esi->id_,ss});


    std::string path=it->second->url.path();
    std::string host= it->second->url.host()+":" + (it->second->url.port().size()? it->second->url.port():"80") ;

    std::string key = "dGhlIHNhbXBsZSBub25jZQ=="; // Пример ключа
    std::string request = "GET " + path + " HTTP/1.1\r\n"
                          "Host: " + host + "\r\n"
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Sec-WebSocket-Key: " + key + "\r\n"
                          "Sec-WebSocket-Version: 13\r\n\r\n";

    e->esi->write_(request);
    it->second->awaiting_handshake=true;
    // passEvent(new webSocketClientEvent::Connected(e->esi,e->route));
    return true;
}
int n=0;
bool webSocketClient::Service::StreamRead(const socketEvent::StreamRead* e)
{
    // logErr2("rd %s",e->esi->inBuffer_._mx_data.c_str());
    auto it=states.find(e->esi->id_);
    if(it==states.end())
        throw CommonError("if(it==states.end())");
    //HTTP/1.1 101 Switching Protocols
// Upgrade: websocket
// Connection: Upgrade
// Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
    if(it->second->awaiting_handshake)
    {
        std::string head;
        bool connected=false;
        {
            W_LOCK(e->esi->inBuffer_.lk);

            auto pz=e->esi->inBuffer_._mx_data.find("\r\n\r\n");
            if(pz!=std::string::npos)
            {
                head=e->esi->inBuffer_._mx_data.substr(0,pz);
                if(e->esi->inBuffer_._mx_data.size()==pz+4)
                {
                    e->esi->inBuffer_._mx_data.clear();
                }
                else
                {
                    e->esi->inBuffer_._mx_data=e->esi->inBuffer_._mx_data.substr(pz+4);
                }

            }

        }

        if(head.find("Connection: Upgrade")!=std::string::npos)
        {
            it->second->awaiting_handshake=false;
            connected=true;
        }
        if(connected)
        {
            passEvent(new webSocketClientEvent::Connected(e->esi,e->route));
        }
    }
    else
    {
        std::string msg;
        {
            W_LOCK(e->esi->inBuffer_.lk);
            auto& buffer=e->esi->inBuffer_._mx_data;
            if ((buffer[0] & 0x0F) == 0x01) { // Текстовый фрейм
                size_t payload_length = buffer[1] & 0x7F;
                msg=std::string(buffer.data() + 2, payload_length);
                buffer=buffer.substr(2+payload_length);
            }
        }
        // n++;
        // if(n%10000==0)
        //     logErr2("client recv %s",msg.c_str());

        passEvent(new webSocketClientEvent::Received(e->esi,msg,e->route));
    }

    return true;
}

bool webSocketClient::Service::Disconnected(const socketEvent::Disconnected* e)
{
    passEvent(new webSocketClientEvent::Disconnected(e->esi,e->route));
    return true;
}


bool webSocketClient::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    auto &ID=e->id;

    if( systemEventEnum::startService==ID)
        return on_startService(static_cast<const systemEvent::startService*>(e.get()));
    if( webSocketClientEventEnum::Connect==ID)
        return Connect(static_cast<const webSocketClientEvent::Connect*>(e.get()));
    if( webSocketClientEventEnum::Send==ID)
        return Send(static_cast<const webSocketClientEvent::Send*>(e.get()));
    // if( rpcEventEnum::IncomingOnAcceptor==ID)
    //     return(this->on_IncomingOnAcceptor(static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get())));
    if( socketEventEnum::Connected==ID)
        return Connected(static_cast< socketEvent::Connected*>(e.get()));
    if( socketEventEnum::ConnectFailed==ID)
        return ConnectFailed(static_cast<const socketEvent::ConnectFailed*>(e.get()));
    if( socketEventEnum::Disconnected==ID)
        return Disconnected(static_cast<const socketEvent::Disconnected*>(e.get()));
    if( socketEventEnum::NotifyOutBufferEmpty==ID)
        return NotifyOutBufferEmpty(static_cast<const socketEvent::NotifyOutBufferEmpty*>(e.get()));
    if( socketEventEnum::StreamRead==ID)
        return StreamRead(static_cast<const socketEvent::StreamRead*>(e.get()));

    logErr2("webSocketClient: unhandled event %s %s %d",iUtils->genum_name(e->id),__FILE__,__LINE__);
    XPASS;
    return false;
}
