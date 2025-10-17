#pragma once

#include "event_mt.h"
#include <IInstance.h>
#include <listenerBuffered1Thread.h>
#include <broadcaster.h>
#include <timerHelper.h>
#include <Events/System/Run/startServiceEvent.h>
#include <Events/Tools/telnetEvent.h>
#include <Events/System/Net/rpcEvent.h>
#include <Events/System/timerEvent.h>
#include <Events/Tools/webHandlerEvent.h>
#include <Events/DFS/referrerEvent.h>
#include "neighbours.h"
#include "uriReferals.h"
#include "uplinkconnectionstate.h"

#define T_CAPS "caps"

namespace refTimer {
    enum {
        T_001_common_connect_failed=1001,
        T_002_D3_caps_get_service_request_is_timed_out=1002,
        T_007_D6_resend_ping_caps_short=1007,
        T_008_D6_resend_ping_caps_long=1008,
        T_009_pong_timed_out_caps_long=1009,
        T_011_downlink_ping_timed_out=1011,
        T_012_reregister_referrer=1012,
        T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers=1020,
        T_040_D2_cache_pong_timed_out_from_neighbours=1040,

    };
}

using namespace  refTimer;

#define CLOSE_DOWNLINKS


struct _dfsReferrerData
{
    _dfsReferrerData(IInstance* instance):uplinkConnectionState(NULL),
        urirefs(new dfsReferrer::_uriReferals()),
        d2_start_time(0),externalAccessIsPossible(false)
        ,
        connection_sequence_id(0),
        neighbours(instance->getName())
    {}
    REF_getter<dfsReferrer::_uplinkConnectionState> uplinkConnectionState;
    REF_getter<dfsReferrer::_uriReferals> urirefs;
    time_t d2_start_time;
    std::set<route_t> m_readyNotificationBackroutes;
    bool externalAccessIsPossible;
    std::string config_bod;
    int connection_sequence_id;
    _neighbours neighbours;
    enum _stage
    {
        /*
            Кэшем не пользуемся, поскольку реферреров не будет много.
            ^STATE_D2_PING_NEIGHBOURS
            Взводим таймер - T_001_common_connect_failed
            Сначала пингуем капсы из конфига. Взводим referrers_timed_out
            Первый понг от капса - посылаем ему getReferres.
            ^STATE_D3_GET_GEFERRERS
            Пингуем полученные рефферы.
            Первый понг от реферрера ^STATE_D5_CHECK_EXTERNAL - посылаем CHECK_EXTERNAL_CONNECTION взводим CHECK_EXTERNAL_CONNECTION_TIMED_OUT
            Сработал referrers_timed_out ^STATE_D5_CHECK_EXTERNAL посылаем на понговый капс CHECK_EXTERNAL_CONNECTION взводим CHECK_EXTERNAL_CONNECTION_TIMED_OUT

            CHECK_EXTERNAL_CONNECTION_TIMED_OUT - exception невозможно стартовать реферрер.

            EXTERNAL_CONNECTION_CHECKED - запускаем ^STATE_D6_MAINTAIN_CONNECTION

            MAINTAIN_CONNECTION fail - ^STATE_D2_PING_NEIGHBOURS

            T_001_common_connect_failed - ^STATE_D2_PING_NEIGHBOURS

        */
        STAGE_D2_PING_NEIGHBOURS,
        STAGE_D21_PING_CAPS,
        STAGE_D3_GET_GEFERRERS,
        STAGE_D3_1_SEND_PING_TO_RESPONDED_REFERRERS,
        STAGE_D4_MAINTAIN_CONNECTION,
        STAGE_D5_CHECK_EXTERNAL,
        STAGE_D6_MAINTAIN_CONNECTION,

    };
    _stage stage;

};

namespace dfsReferrer
{


    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster,
        public TimerHelper
    {

        _dfsReferrerData rd;
        msockaddr_in getUL();
        bool hasUL();

        bool on_startService(const systemEvent::startService*);
    public:
        bool handleEvent(const REF_getter<Event::Base>& e);

        bool on_CommandEntered(const telnetEvent::CommandEntered*e);

        bool on_ConnectFailed(const rpcEvent::ConnectFailed*);
        bool on_Disconnected(const rpcEvent::Disconnected*e);
        bool on_Disaccepted(const rpcEvent::Disaccepted*e);
        bool on_Connected(const rpcEvent::Connected*);
        bool on_IncomingOnConnector(const rpcEvent::IncomingOnConnector*);
        bool on_IncomingOnAcceptor(const rpcEvent::IncomingOnAcceptor*);
        bool on_Accepted(const rpcEvent::Accepted*);
        bool on_Binded(const rpcEvent::Binded*);



        bool on_TickTimer(const timerEvent::TickTimer*);
        bool on_TickAlarm(const timerEvent::TickAlarm*);
#ifdef WEBDUMP
        bool on_RequestIncoming(const webHandlerEvent::RequestIncoming*);
#endif
        bool on_UpdateConfigREQ(const dfsReferrerEvent::UpdateConfigREQ*);
        bool on_UpdateConfigRSP(const dfsReferrerEvent::UpdateConfigRSP*);

        void on_deleteDownlink(const SOCKET_id& sock, const msockaddr_in& remote_name);


        void _reset_alarm(int alarm_id);
        void _reset_alarm(int alarm_id, const msockaddr_in& sa);
        void _reset_alarm(int alarm_id1,int alarm_id2, const msockaddr_in& sa);
        void _stop_alarm(int alarm_id);
        void _stop_alarm(int alarm_id, const msockaddr_in& sa);
        void _stop_alarm(int alarm_id1,int alarm_id2, const msockaddr_in& sa);
        void _stop_alarm(int alarm_id1,int alarm_id2,int alarm_id3, const msockaddr_in& sa);

        void reset_T_001_common_connect_failed();



        void STAGE_D2_PING_NEIGHBOURS_start();
        void STAGE_D21_PING_CAPS_start();




        void STAGE_D3_GET_GEFERRERS_start(const msockaddr_in& sa);



        void d4_on_disconnected(const msockaddr_in& sa);
        void d4_on_connect_failed(const msockaddr_in& sa);
        void d4_uplink_mode(const REF_getter<epoll_socket_info> &esi,const msockaddr_in& visibleName);

        void d6_start(const msockaddr_in& sa);
        bool d6_on_pong_PT_CAPS_SHORT(const REF_getter<epoll_socket_info>&);
        bool d6_on_pong_PT_CAPS_LONG(const REF_getter<epoll_socket_info>&);
        void d6_on_T_011_resend_ping_PT_CAPS_LONG(const msockaddr_in& remote_addr);
        void d6_on_T_012_pong_timed_out_PT_CAPS_LONG(const msockaddr_in& remote_addr);
        void d6_on_disconnected(const msockaddr_in& remote_addr);
        void d6_on_connect_failed(const msockaddr_in& remote_addr);


        bool on_connectionEstablished(const msockaddr_in &remote_addr);

        bool  on_SubscribeNotifications(const dfsReferrerEvent::SubscribeNotifications*e);


        bool on_ToplinkDeliverREQ(const dfsReferrerEvent::ToplinkDeliverREQ *e, const rpcEvent::IncomingOnAcceptor* acc);
        bool on_ToplinkDeliverRSP(const dfsReferrerEvent::ToplinkDeliverRSP *e);

        bool on_Pong(const dfsReferrerEvent::Pong* e,  const REF_getter<epoll_socket_info>& esi);

        bool on_Ping(const dfsReferrerEvent::Ping* e, const REF_getter<epoll_socket_info>& esi);


        void sendToplinkReq(const msockaddr_in &uplink, dfsReferrerEvent::ToplinkDeliverREQ *ee);


        void kill_downlinks();

        void registerReferrer(const msockaddr_in &destination);

        bool isTopServer(const msockaddr_in& sa);

    public:
        Service(const SERVICE_id &svs, const std::string&  nm,IInstance* ifa);
        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa);
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        ~Service();

        IInstance *iInstance;
    };

}

