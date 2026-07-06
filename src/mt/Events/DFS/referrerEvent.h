#pragma once
#include "EVENT_id.h"
#include "GlobalCookie_id.h"
#include "IUtils.h"
#include "SERVICE_id.h"
#include "event_mt.h"
#include "mutexInspector.h"

#include "ghash.h"
namespace ServiceEnum
{
    const SERVICE_id DFSReferrer(ghash("@g_DFSReferrer"));
    const SERVICE_id ReferrerClient(ghash("@g_RefferrerClient"));
}

namespace dfsReferrerEventEnum
{

    const EVENT_id Ping(ghash("@g_DFSReferrer_Ping"));
    const EVENT_id Hello(ghash("@g_DFSReferrer_Hello"));
    const EVENT_id SubscribeNotifications(ghash("@g_DFSReferrer_SubscribeNotifications"));
    const EVENT_id Noop(ghash("@g_DFSReferrer_Noop"));

    const EVENT_id ToplinkDeliverREQ(ghash("@g_DFSReferrer_ToplinkDeliverREQ"));
    const EVENT_id ToplinkDeliverRSP(ghash("@g_DFSReferrer_ToplinkDeliverRSP"));



    const EVENT_id Pong(ghash("@g_DFSReferrer_Pong"));
    const EVENT_id NotifyReferrerUplinkIsConnected(ghash("@g_DFSReferrerNotifyReferrerUplinkIsConnected"));
    const EVENT_id NotifyReferrerUplinkIsDisconnected(ghash("@g_DFSReferrerNotifyReferrerUplinkIsDisconnected"));
    const EVENT_id NotifyReferrerDownlinkDisconnected(ghash("@g_DFSReferrerNotifyReferrerDownlinkDisconnected"));
    const EVENT_id InitClient(ghash("@g_DFSReferrerInitClient"));

    const EVENT_id UpdateConfigREQ(ghash("@g_DFSReferrerUpdateConfigREQ"));
    const EVENT_id UpdateConfigRSP(ghash("@g_DFSReferrerUpdateConfigRSP"));

}
namespace dfsReferrer {


    namespace PingType {
        enum
        {
            PT_CACHED,
            PT_MASTER_SHORT,
            PT_MASTER_LONG,
            PT_CAPS_SHORT,
            PT_CAPS_LONG,
        };
    }
}

namespace dfsReferrerEvent {
    class InitClient: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        InitClient(const std::set<msockaddr_in>& _caps)
            :NoPacked(dfsReferrerEventEnum::InitClient),caps(_caps) {}

        std::set<msockaddr_in>caps;
    };

    class NotifyReferrerDownlinkDisconnected: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        NotifyReferrerDownlinkDisconnected(const SOCKET_id& _sid, const route_t& r)
            :NoPacked(dfsReferrerEventEnum::NotifyReferrerDownlinkDisconnected,r),socketId(_sid) {}

        const SOCKET_id& socketId;
    };
    class NotifyReferrerUplinkIsConnected: public Event::NoPacked
    {
        /**
            Clean old uplinks and replace by new
          */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        NotifyReferrerUplinkIsConnected(const msockaddr_in& _sa,const route_t& r)
            :NoPacked(dfsReferrerEventEnum::NotifyReferrerUplinkIsConnected,r),sa(_sa) {}

        msockaddr_in sa;
    };
    class NotifyReferrerUplinkIsDisconnected: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        NotifyReferrerUplinkIsDisconnected(const msockaddr_in& _sa,const route_t& r)
            :NoPacked(dfsReferrerEventEnum::NotifyReferrerUplinkIsDisconnected,r), sa(_sa) {}

        msockaddr_in sa;

    };

    class Ping: public Event::Base
    {

    public:
        enum ClientType
        {
            CT_NODE,
            CT_CLIENT
        };
        static Base* construct(const route_t &r)
        {
            return new Ping(r);
        }
        int pingType;
        GlobalCookie_id globalCookieOfSender;
        unsigned short externalListenPort;
        std::set<msockaddr_in> internalListenAddr;
        int64_t ping_time;
        int connection_sequence_id;
        int clientType;
        Ping(const int& _type, const GlobalCookie_id& _globalCookie, unsigned short _externalListenPort, const std::set<msockaddr_in>& _internalListenAddr, int64_t _ping_time, int _connection_sequence_id, int _clientType,const route_t &r)
            :Base(dfsReferrerEventEnum::Ping,r),
             pingType(_type),globalCookieOfSender(_globalCookie),externalListenPort(_externalListenPort),internalListenAddr(_internalListenAddr),ping_time(_ping_time),connection_sequence_id(_connection_sequence_id),clientType(_clientType)
        {}

        Ping(const int& _type, const GlobalCookie_id& _globalCookie,
             int64_t _ping_time, int _connection_sequence_id, int _clientType,const route_t &r)
            :Base(dfsReferrerEventEnum::Ping,r),
             pingType(_type),globalCookieOfSender(_globalCookie),externalListenPort(0),ping_time(_ping_time),connection_sequence_id(_connection_sequence_id),clientType(_clientType)
        {}

        Ping(const route_t& r)
            :Base(dfsReferrerEventEnum::Ping,r) {}
        void unpack(inBuffer& o)
        {

            o>>pingType>>globalCookieOfSender>>externalListenPort>>internalListenAddr>>ping_time>>connection_sequence_id>>clientType;
        }
        void pack(outBuffer& o) const
        {

            o<<pingType<<globalCookieOfSender<<externalListenPort<<internalListenAddr<<ping_time<<connection_sequence_id<<clientType;
        }

    };
    class Pong: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new Pong(r);
        }
        int pingType;
        msockaddr_in visible_name_of_pinger;
        GlobalCookie_id globalCookieOfResponder;
        int nodeLevelInHierarhy;
        int64_t ping_time;
        int connection_sequence_id;
        Pong(const int& _type,const msockaddr_in& _visible_name_of_pinger,const GlobalCookie_id& _globalCookieOfResponder, const int& nodeLevelInHierarhy_, int64_t _ping_time, int _connection_sequence_id,const route_t& r)
            :Base(dfsReferrerEventEnum::Pong,r),
             pingType(_type),visible_name_of_pinger(_visible_name_of_pinger),globalCookieOfResponder(_globalCookieOfResponder),
             nodeLevelInHierarhy(nodeLevelInHierarhy_),ping_time(_ping_time),connection_sequence_id(_connection_sequence_id)
        {}
        Pong(const route_t& r)
            :Base(dfsReferrerEventEnum::Pong,r) {}
        void unpack(inBuffer& o)
        {

            o>>pingType>>visible_name_of_pinger>>globalCookieOfResponder>>nodeLevelInHierarhy>>ping_time>>connection_sequence_id;
        }
        void pack(outBuffer&o) const
        {

            o<<pingType<<visible_name_of_pinger<<globalCookieOfResponder<<nodeLevelInHierarhy<<ping_time<<connection_sequence_id;
        }

    };

}


/**
Доставка события вверх по облаку. На некотором уровне оно форвардится в сервис, согласно общей конфигурации ботов.

Eсли destination service == "broadcast", то делаем бродкаст по облаку.
*/
namespace dfsReferrerEvent {
    class ToplinkDeliverREQ: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new ToplinkDeliverREQ(r);
        }
        ToplinkDeliverREQ(const SERVICE_id& _destinationService, const REF_getter<Event::Base>& e, const route_t& r)
            :Base(dfsReferrerEventEnum::ToplinkDeliverREQ,r),
             destinationService(_destinationService),eventData(NULL)
        {
            outBuffer o;
            iUtils->packEvent(o,e);
            eventData=o.asString();
        }
        ToplinkDeliverREQ(const route_t& r)
            :Base(dfsReferrerEventEnum::ToplinkDeliverREQ,r),eventData(NULL)
        {
            if(!eventData.valid())
                eventData=new refbuffer;

        }
        SERVICE_id destinationService;
        REF_getter<refbuffer> eventData;

        std::set<GlobalCookie_id> uuid;

        REF_getter<Event::Base> getEvent() const
        {
            MUTEX_INSPECTOR;
            inBuffer in(eventData);
            REF_getter<Event::Base> z=iUtils->unpackEvent(in);
            return z;
        }
        void unpack(inBuffer& o)
        {
            o>>destinationService>>eventData>>uuid;
        }
        void pack(outBuffer&o) const
        {
            o<<destinationService<<eventData<<uuid;
        }

    };
    class ToplinkDeliverRSP: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {

            return new ToplinkDeliverRSP(r);
        }
        ToplinkDeliverRSP(const REF_getter<Event::Base>& _e,const route_t& r)
            :Base(dfsReferrerEventEnum::ToplinkDeliverRSP,r),
             eventData(NULL)
        {
            outBuffer o;
            iUtils->packEvent(o,_e);
            eventData=o.asString();

        }
        ToplinkDeliverRSP(const route_t& r)
            :Base(dfsReferrerEventEnum::ToplinkDeliverRSP,r),eventData(NULL)
        {
            if(!eventData.valid())
                eventData=new refbuffer;

        }
        REF_getter<refbuffer> eventData;
        void unpack(inBuffer& o)
        {

            o>>eventData;
        }
        void pack(outBuffer&o) const
        {

            o<<eventData;
        }
        REF_getter<Event::Base> getEvent() const
        {
            MUTEX_INSPECTOR;
            inBuffer in(eventData);
            REF_getter<Event::Base> z=iUtils->unpackEvent(in);
            return z;
        }

    };


    class SubscribeNotifications: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new SubscribeNotifications(r);
        }
        SubscribeNotifications(const route_t& r)
            :Base(dfsReferrerEventEnum::SubscribeNotifications,r) {}

        void unpack(inBuffer&)
        {
        }
        void pack(outBuffer&) const
        {
        }
    };


    class UpdateConfigREQ: public Event::Base
    {


    public:
        static Base* construct(const route_t &r)
        {
            return new UpdateConfigREQ(r);
        }
        UpdateConfigREQ(const route_t& r)
            :Base(dfsReferrerEventEnum::UpdateConfigREQ,r) {}
        UpdateConfigREQ(const std::string &_body,  const route_t &r)
            :Base(dfsReferrerEventEnum::UpdateConfigREQ,r),
             bod(_body) {}
        std::string bod;
        void unpack(inBuffer& o)
        {

            o>>bod;
        }
        void pack(outBuffer&o) const
        {

            o<<bod;
        }
    };

    class UpdateConfigRSP: public Event::Base
    {


    public:
        static Base* construct(const route_t &r)
        {
            return new UpdateConfigRSP(r);
        }
        UpdateConfigRSP(const route_t& r)
            :Base(dfsReferrerEventEnum::UpdateConfigRSP,r) {}
        UpdateConfigRSP(const std::string &_body,  const route_t &r)
            :Base(dfsReferrerEventEnum::UpdateConfigRSP,r),
             bod(_body) {}
        std::string bod;
        void unpack(inBuffer& o)
        {

            o>>bod;
        }
        void pack(outBuffer&o) const
        {

            o<<bod;
        }
    };
}

