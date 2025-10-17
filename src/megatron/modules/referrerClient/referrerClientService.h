#pragma once
#include "listenerBuffered1Thread.h"
#include "broadcaster.h"
#include "REF.h"
#include "route_t.h"
#include "msockaddr_in.h"
#include "neighbours.h"
#include "uplinkconnectionstate_client.h"
#include "timerHelper.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/Tools/telnetEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/DFS/referrerEvent.h"



namespace crefTimer {
    enum {
        T_001_common_connect_failed=1001,
        T_002_D3_caps_get_service_request_is_timed_out=1002,
        T_007_D6_resend_ping_caps_short=1007,
        T_008_D6_resend_ping_caps_long=1008,
        T_009_pong_timed_out_caps_long=1009,
        T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers=1020,
        T_040_D2_cache_pong_timed_out_from_neighbours=1040,

    };
}

using namespace  crefTimer;

#define CLOSE_DOWNLINKS

namespace referrerClient
{


    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster,
        public TimerHelper
    {

        REF_getter<_uplinkConnectionState> uplinkConnectionState;


        msockaddr_in getUL();
        bool hasUL();




        bool on_startService(const systemEvent::startService*);

    public:

        bool handleEvent(const REF_getter<Event::Base>& e);

        bool on_CommandEntered(const telnetEvent::CommandEntered*e);

        bool on_ConnectFailed(const rpcEvent::ConnectFailed*);
        bool on_Disconnected(const rpcEvent::Disconnected*e);
        bool on_Connected(const rpcEvent::Connected*);
        bool on_IncomingOnConnector(const rpcEvent::IncomingOnConnector*);



        bool on_TickTimer(const timerEvent::TickTimer*);
        bool on_TickAlarm(const timerEvent::TickAlarm*);

#ifdef WEBDUMP
        bool on_RequestIncoming(const webHandlerEvent::RequestIncoming*);
#endif



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


        time_t d2_start_time;


        void STAGE_D3_GET_GEFERRERS_start(const msockaddr_in& sa);



        void d4_on_disconnected(const msockaddr_in& sa);
        void d4_on_connect_failed(const msockaddr_in& sa);
        void d4_uplink_mode(const REF_getter<epoll_socket_info> &esi,const msockaddr_in& visibleName);

        /// check external connection
        void d5_start(const REF_getter<epoll_socket_info>&esi,const msockaddr_in& visibleName);

        /// maintain connection with caps
        void d6_start(const msockaddr_in& sa);
        bool d6_on_pong_PT_CAPS_SHORT(const msockaddr_in&visible_name_of_pinger, const REF_getter<epoll_socket_info>&);
        bool d6_on_pong_PT_CAPS_LONG(const msockaddr_in&visible_name_of_pinger, const REF_getter<epoll_socket_info>&);
        void d6_on_T_011_resend_ping_PT_CAPS_LONG(const msockaddr_in& remote_addr);
        void d6_on_T_012_pong_timed_out_PT_CAPS_LONG(const msockaddr_in& remote_addr);
        void d6_on_disconnected(const msockaddr_in& remote_addr);
        void d6_on_connect_failed(const msockaddr_in& remote_addr);


        bool on_connectionEstablished(const msockaddr_in &remote_addr);

        bool  on_SubscribeNotifications(const dfsReferrerEvent::SubscribeNotifications*e);


        bool on_ToplinkDeliverREQ(const dfsReferrerEvent::ToplinkDeliverREQ *e);
        bool on_ToplinkDeliverRSP(const dfsReferrerEvent::ToplinkDeliverRSP *e);

        bool on_Pong(const dfsReferrerEvent::Pong* e,  const REF_getter<epoll_socket_info>& esi);

        bool on_InitClient(const dfsReferrerEvent::InitClient *e);


        void sendToplinkReqClient(const msockaddr_in &uplink, dfsReferrerEvent::ToplinkDeliverREQ *ee);

        std::set<route_t> m_readyNotificationBackroutes;



//        bool isTopServer(const msockaddr_in& sa);

    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        Service(const SERVICE_id &svs, const std::string&  nm,IInstance* ifa);
        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa);
        ~Service();

        IInstance *iInstance;
        ////
        enum _stage
        {
            STAGE_D2_PING_NEIGHBOURS,
            STAGE_D21_PING_CAPS,
            STAGE_D3_GET_GEFERRERS,
            STAGE_D3_1_SEND_PING_TO_RESPONDED_REFERRERS,
            STAGE_D4_MAINTAIN_CONNECTION,
            STAGE_D5_CHECK_EXTERNAL,
            STAGE_D6_MAINTAIN_CONNECTION,

        };
        _stage stage;

        int connection_sequence_id;
        _neighbours neighbours;


        std::string config_bod;
    };

}

