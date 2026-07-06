#include <unistd.h>
#include <version_mega.h>
#include "oscarService.h"

#include "event_mt.h"
#include <Events/System/Net/socketEvent.h>
#include <Events/System/Net/oscarEvent.h>
#include <mutexInspector.h>
#include "events_oscar.hpp"

Oscar::Service::Service(const SERVICE_id &svs, const std::string&  nm,IInstance* ifa):
    UnknownBase(nm),
    ListenerSimple(nm,svs),
    Broadcaster(ifa),
    iInstance(ifa)
{
    try
    {
        XTRY;
        maxPacketSize=ifa->getConfig()->get_int64_t("maxPacketSize",1024*1024*16,"Max packet size in oscar protocol");
        XPASS;
    }
    catch (const std::exception &e)
    {
        logErr2("exception: %s %s %d",e.what(),__FILE__,__LINE__);
        throw;
    }

}

Oscar::Service::~Service()
{
}

bool Oscar::Service::on_Connect(const oscarEvent::Connect* e)
{
    MUTEX_INSPECTOR;
    sendEvent(socketListener,new socketEvent::AddToConnectTCP(e->socketId,e->addr,e->socketDescription, e->secure,e->route));
    return true;
}
bool Oscar::Service::on_SendPacket(const oscarEvent::SendPacket* e)
{
    MUTEX_INSPECTOR;
    sendPacketPlain(Oscar::SB_SINGLEPACKET,e->esi,e->buf);
    return true;
}
bool Oscar::Service::on_AddToListenTCP(const oscarEvent::AddToListenTCP* e)
{
    MUTEX_INSPECTOR;
    sendEvent(socketListener,new socketEvent::AddToListenTCP(e->socketId,e->addr,e->socketDescription,false, e->secure, e->route));
    return true;
}
bool Oscar::Service::on_Accepted(const socketEvent::Accepted*e)
{
    MUTEX_INSPECTOR;

    passEvent(new oscarEvent::Accepted(e->esi,e->route));
    return true;
}
bool Oscar::Service::on_StreamRead(const socketEvent::StreamRead* evt)
{

    MUTEX_INSPECTOR;
    try
    {
        while (1)
        {
            std::string req;
            bool recvd=false;
            bool success=false;
            unsigned char start_byte;
            bool need_disconnect=false;
            {
                {
                    XTRY;
                    {
                        W_LOCK(evt->esi->inBuffer_.lk);
                        if(evt->esi->inBuffer_._mx_data.size()==0)
                            return true;
                        inBuffer b(evt->esi->inBuffer_._mx_data.data(), evt->esi->inBuffer_._mx_data.size());

                        start_byte=b.get_8_nothrow(success);


                        if (!success) return true;
                        {
                            XTRY;
                            auto len= static_cast<size_t>(b.get_PN_nothrow(success));
                            if (!success) return true;
                            {
                                XTRY;
                                if(len>maxPacketSize)
                                {
                                    logErr2("packet size %lu more maxPacketSize %lu",len,maxPacketSize);
                                    return true;

                                }
                                if(b.remains()<len)
                                {
                                    
                                    // logErr2("if(b.remains()<len)");
                                    return true;
                                }

                                b.unpack_nothrow(req,len,success);

                                if (!success)
                                {
                                    logErr2("---------_ERROR if (!success)");
                                    return true;
                                }

                                if(b.remains()==0)
                                {
                                    evt->esi->inBuffer_._mx_data.clear();
                                }
                                else
                                {
                                    evt->esi->inBuffer_._mx_data.erase(0,evt->esi->inBuffer_._mx_data.size()-b.remains());
                                }
                                recvd=true;
                                XPASS;
                            }
                            XPASS;
                        }

                    }
                    XPASS;
                }
            }


            if(need_disconnect)
            {
                XTRY;
                evt->esi->close("oscar buffer broken");
                logErr2("oscar buffer broken");
                return true;
                XPASS;
            }
            if(!recvd) return true;
            {
                XTRY;
                switch(start_byte)
                {
                case Oscar::SB_SINGLEPACKET:
                {

                    if(evt->esi->streamType_==epoll_socket_info::STREAMTYPE_CONNECTED)
                    {
                        passEvent(new oscarEvent::PacketOnConnector(evt->esi,toRef(std::move(req)),evt->route));
                    }
                    else
                    {
                        passEvent(new oscarEvent::PacketOnAcceptor(evt->esi,toRef(std::move(req)),evt->route));
                    }
                    continue;
                }
                break;
                default:
                    logErr2("Invalid start byte %d",start_byte);
                    evt->esi->close("oscar: invalid start byte");

                    return true;
                }
                XPASS;
            }
            break;
        }
    }
    catch (const std::exception &e)
    {
        logErr2("exception: %s (%s %d)",e.what(),__FILE__,__LINE__);
        evt->esi->close(e.what());
    }
    catch (...)
    {
        logErr2("exception: unknown (%s %d)",__FILE__,__LINE__);
        evt->esi->close("exeption ...");
    }
    return true;
}
bool Oscar::Service::on_Connected(const socketEvent::Connected* e)
{
    MUTEX_INSPECTOR;
    passEvent(new oscarEvent::Connected(e->esi,e->route));
    return true;
}
UnknownBase* Oscar::Service::construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
{
    XTRY;
    return new Service(id,nm,ifa);
    XPASS;
}
bool Oscar::Service::on_NotifyOutBufferEmpty(const socketEvent::NotifyOutBufferEmpty* e)
{
    MUTEX_INSPECTOR;
    passEvent(new oscarEvent::NotifyOutBufferEmpty(e->esi,e->route));
    return true;
}
bool Oscar::Service::on_NotifyBindAddress(const socketEvent::NotifyBindAddress*e)
{
    MUTEX_INSPECTOR;
    passEvent(new oscarEvent::NotifyBindAddress(e->addr,e->socketDescription,e->rebind,e->route));
    return true;
}
#include "obuf.h"
void Oscar::Service::sendPacketPlain(const Oscar::StartByte& startByte, const REF_getter<epoll_socket_info>& esi, const outBuffer &o)
{
    MUTEX_INSPECTOR;
    XTRY;
    size_t bs=o.size()+0x100;
    char b[bs];
    oBuf O2(b,bs);
    O2.put_8(startByte);
    O2<<o.asString();
    esi->write_buf(O2.data(), O2.size());
    XPASS;
}
void Oscar::Service::sendPacketPlain(const Oscar::StartByte& startByte, const REF_getter<epoll_socket_info>& esi, const REF_getter<refbuffer> &o)
{
    MUTEX_INSPECTOR;
    XTRY;
    if(!esi.valid())
        throw CommonError("if(!esi.valid()) %s",__PRETTY_FUNCTION__);
    size_t bs=o->container.size()+0x100;
    char b[bs];
    oBuf O2(b,bs);
    O2.put_8(startByte);
    O2<<o;
    esi->write_buf(O2.data(), O2.size());
    XPASS;
}
bool Oscar::Service::on_startService(const systemEvent::startService*)
{
    socketListener=dynamic_cast<ListenerBase*>(iInstance->getServiceOrCreate(ServiceEnum::Socket));
    return true;
}

void registerOscarModule(const char* pn)
{
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::Oscar,"Oscar",getEvents_oscar());
    }
    else
    {
        iUtils->registerService(ServiceEnum::Oscar,Oscar::Service::construct,"Oscar");
        regEvents_oscar();
    }

}


bool Oscar::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    auto &ID=e->id;
    switch(ID)
    {
    case socketEventEnum::Accepted:
        return on_Accepted((const socketEvent::Accepted*)e.get());
    case socketEventEnum::StreamRead:
        return on_StreamRead((const socketEvent::StreamRead*)e.get());
    case socketEventEnum::Connected:
        return on_Connected((const socketEvent::Connected*)e.get());
    case socketEventEnum::NotifyBindAddress:
        return on_NotifyBindAddress((const socketEvent::NotifyBindAddress*)e.get());
    case socketEventEnum::NotifyOutBufferEmpty:
        return on_NotifyOutBufferEmpty((const socketEvent::NotifyOutBufferEmpty*)e.get());
    case socketEventEnum::ConnectFailed:
        return on_ConnectFailed((const socketEvent::ConnectFailed*)e.get());
    case oscarEventEnum::SendPacket:
        return(this->on_SendPacket((const oscarEvent::SendPacket*)e.get()));

    case oscarEventEnum::AddToListenTCP:
        return(this->on_AddToListenTCP((const oscarEvent::AddToListenTCP*)e.get()));

    case oscarEventEnum::Connect:
        return(this->on_Connect((const oscarEvent::Connect*)e.get()));

    case systemEventEnum::startService:
        return on_startService((const systemEvent::startService*)e.get());


    case socketEventEnum::Disaccepted:
        return on_Disaccepted((const socketEvent::Disaccepted*)e.get());
    case socketEventEnum::Disconnected:
        return on_Disconnected((const socketEvent::Disconnected*)e.get());
    }
    XPASS;
    return false;

}
bool Oscar::Service::on_ConnectFailed(const socketEvent::ConnectFailed*e)
{
    passEvent(new oscarEvent::ConnectFailed(e->esi,e->addr,e->route));
    return true;
}
bool Oscar::Service::on_Disaccepted(const socketEvent::Disaccepted*e)
{
    passEvent(new oscarEvent::Disaccepted(e->esi,e->reason,e->route));
    return true;
}
bool Oscar::Service::on_Disconnected(const socketEvent::Disconnected*e)
{
    passEvent(new oscarEvent::Disconnected(e->esi,e->reason,e->route));
    return true;
}
