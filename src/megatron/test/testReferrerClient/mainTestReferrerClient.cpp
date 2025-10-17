#include "IInstance.h"
#include "colorOutput.h"
#include "configObj.h"
#include "CUtils.h"
#include "appHandler.h"
#include "Events/DFS/capsEvent.h"
#include <unistd.h>
void registerRPCService(const char* pn);
void registerSocketModule(const char* pn);
void registerTimerService(const char* pn);
void registerSSL(const char* pn);
void registerOscarModule(const char* pn);
// void registerOscarSecureModule(const char* pn);
void registerDFSReferrerService(const char* pn);
void registerDFSCapsService(const char* pn);
void registerGEOIP(const char* pn);
void registerReferrerClientService(const char* pn);
void registerObjectProxyModule(const char* pn);
#ifdef WEBDUMP
void registerWebHandlerModule(const char* pn);
#endif
int mainTestReferrerClient(int argc, char** argv )
{
    try {
        iUtils=new CUtils(argc, argv, "referrerClientTest");

        registerRPCService(NULL);
        registerSocketModule(NULL);
        registerTimerService(NULL);
        registerSSL(NULL);
        registerOscarModule(NULL);
        // registerOscarSecureModule(NULL);
        registerDFSReferrerService(NULL);
        registerDFSCapsService(NULL);
        registerGEOIP(NULL);
        registerReferrerClientService(NULL);
        registerObjectProxyModule(NULL);
#ifdef WEBDUMP
        registerWebHandlerModule(NULL);
#endif
        iUtils->registerEvent(dfsCapsEvent::RegisterMyRefferrerNodeREQ::construct);

        printf(GREEN("RUN TEST %s"),__PRETTY_FUNCTION__);

        {

            const char* cf=R"zxc(
# list of initially started services
Start=RPC,DFSReferrer,DFSCaps

# Address used to work with dfs network. NONE - no bind
RPC_BindAddr_MAIN=INADDR_ANY:10100,INADDR6_ANY:0

# Address used to communicate with local apps, must be fixed. NONE - no bind
RPC_BindAddr_RESERVE=NONE
SocketIO_epoll_timeout_millisec=10
SocketIO_listen_backlog=128
RPC_oscarType=Oscar

# socket poll thread count
SocketIO_n_workers=2

# Is caps mode. Do not forward caps requests to uplink
DFSReferrer_IsCapsServer=true
DFSReferrer_T_001_common_connect_failed=20.000000
DFSReferrer_T_002_D3_caps_get_service_request_is_timed_out=15.000000
DFSReferrer_T_007_D6_resend_ping_caps_short=7.000000
DFSReferrer_T_008_D6_resend_ping_caps_long=20.000000
DFSReferrer_T_009_pong_timed_out_caps_long=40.000000
DFSReferrer_T_011_downlink_ping_timed_out=60.000000
DFSReferrer_T_012_reregister_referrer=3500.000000
DFSReferrer_T_020_D3_1_wait_after_send_PT_CACHE_on_recvd_from_GetService=2.000000
DFSReferrer_T_004_cache_pong_timed_out_=2.000000

# Addr:port to bind. Addr=INADDR_ANY:port - bind to all interfaces. NONE - no bind
WebHandler.bindAddr=NONE

)zxc";
            IInstance *instance1=iUtils->createNewInstance("root");
            ConfigObj *cnf1=new ConfigObj("root.conf",cf);
            instance1->setConfig(cnf1);
            instance1->initServices();
            sleep(1);
        }
        for(int i=0; i<2; i++)
        {

            std::string cf[]={
                                 R"zxc(

# list of initially started services
Start=RPC,DFSReferrer

# Address used to work with dfs network. NONE - no bind
RPC_BindAddr_MAIN=INADDR_ANY:10101,INADDR6_ANY:10101

# Address used to communicate with local apps, must be fixed. NONE - no bind
RPC_BindAddr_RESERVE=NONE
SocketIO_epoll_timeout_millisec=10
SocketIO_listen_backlog=128

# socket poll thread count
SocketIO_n_workers=2

# Is caps mode. Do not forward caps requests to uplink
DFSReferrer_IsCapsServer=false
DFSReferrer_CapsServerUrl=127.0.0.1:10100
DFSReferrer_T_001_common_connect_failed=20.000000
DFSReferrer_T_002_D3_caps_get_service_request_is_timed_out=15.000000
DFSReferrer_T_007_D6_resend_ping_caps_short=7.000000
DFSReferrer_T_008_D6_resend_ping_caps_long=20.000000
DFSReferrer_T_009_pong_timed_out_caps_long=40.000000
DFSReferrer_T_011_downlink_ping_timed_out=60.000000
DFSReferrer_T_012_reregister_referrer=3500.000000
DFSReferrer_T_020_D3_1_wait_after_send_PT_CACHE_on_recvd_from_GetService=2.000000
DFSReferrer_T_004_cache_pong_timed_out_=2.000000

# Addr:port to bind. Addr=INADDR_ANY:port - bind to all interfaces. NONE - no bind
WebHandler.bindAddr=NONE
)zxc"
                                 ,
                                 R"zxc(

# list of initially started services
Start=RPC,DFSReferrer

# Address used to work with dfs network. NONE - no bind
RPC_BindAddr_MAIN=INADDR_ANY:10102,INADDR6_ANY:10102

# Address used to communicate with local apps, must be fixed. NONE - no bind
RPC_BindAddr_RESERVE=NONE
SocketIO_epoll_timeout_millisec=10
SocketIO_listen_backlog=128

# socket poll thread count
SocketIO_n_workers=2

# Is caps mode. Do not forward caps requests to uplink
DFSReferrer_IsCapsServer=false
DFSReferrer_CapsServerUrl=127.0.0.1:10100
DFSReferrer_T_001_common_connect_failed=20.000000
DFSReferrer_T_002_D3_caps_get_service_request_is_timed_out=15.000000
DFSReferrer_T_007_D6_resend_ping_caps_short=7.000000
DFSReferrer_T_008_D6_resend_ping_caps_long=20.000000
DFSReferrer_T_009_pong_timed_out_caps_long=40.000000
DFSReferrer_T_011_downlink_ping_timed_out=60.000000
DFSReferrer_T_012_reregister_referrer=3500.000000
DFSReferrer_T_020_D3_1_wait_after_send_PT_CACHE_on_recvd_from_GetService=2.000000
DFSReferrer_T_004_cache_pong_timed_out_=2.000000

# Addr:port to bind. Addr=INADDR_ANY:port - bind to all interfaces. NONE - no bind
WebHandler_bindAddr=NONE

                )zxc"
        };
        IInstance *instance1=iUtils->createNewInstance("node_"+std::to_string(i));

        ConfigObj *cnf1=new ConfigObj("node_"+std::to_string(i)+".conf",cf[i]);
        instance1->setConfig(cnf1);
        instance1->initServices();
        sleep(1);

    }
    std::set<AppHandler*> apps;
    for(int i=0; i<1; i++)
    {

        const char *cf=R"zxc(
ReferrerClient_T_001_common_connect_failed=20.000000
ReferrerClient_T_002_D3_caps_get_service_request_is_timed_out=15.000000
ReferrerClient_T_007_D6_resend_ping_caps_short=7.000000
ReferrerClient_T_008_D6_resend_ping_caps_long=20.000000
ReferrerClient_T_009_pong_timed_out_caps_long=40.000000
ReferrerClient_T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers=2.000000
ReferrerClient_T_040_D2_cache_pong_timed_out_from_neighbours=2.000000

# Address used to work with dfs network. NONE - no bind
RPC_BindAddr_MAIN=INADDR_ANY:0,INADDR6_ANY:0

# Address used to communicate with local apps, must be fixed. NONE - no bind
RPC_BindAddr_RESERVE=NONE
SocketIO_epoll_timeout_millisec=10
SocketIO_listen_backlog=128

# socket poll thread count
SocketIO_n_workers=2
SocketIO_epoll_timeout_millisec=10
SocketIO_listen_backlog=128

# socket poll thread count
SocketIO_n_workers=2
SocketIO_epoll_timeout_millisec=10

# Addr:port to bind. Addr=INADDR_ANY:port - bind to all interfaces. NONE - no bind
WebHandler.bindAddr=NONE

# Address used to work with dfs network. NONE - no bind
RPC_BindAddr_MAIN=INADDR_ANY:0,INADDR6_ANY:0

# Address used to communicate with local apps, must be fixed. NONE - no bind
RPC_BindAddr_RESERVE=NONE

# Address used to work with dfs network. NONE - no bind
RPC_BindAddr_MAIN=INADDR_ANY:0,INADDR6_ANY:0

# Address used to communicate with local apps, must be fixed. NONE - no bind
RPC_BindAddr_RESERVE=NONE
Socket.epoll_timeout_millisec=10
Socket.listen_backlog=128

# socket poll thread count
Socket.n_workers=2

# Root cloud address
CapsIP=127.0.0.1:10101

)zxc";
            IInstance *instance1=iUtils->createNewInstance("client_"+std::to_string(i));
            ConfigObj *cnf1=new ConfigObj("client.conf",cf);
            instance1->setConfig(cnf1);
            AppHandler * h=new AppHandler(instance1);
            apps.insert(h);
            h->init(argc,argv,"",instance1);
            sleep(1);
        }
        while(!iUtils->isTerminating())
        {
            sleep(1);
        }
        for(auto& z:apps)
            delete z;
        delete iUtils;
        return 0;

    } catch (CommonError& e)
    {
        printf("CommonError %s\n",e.what());
    }
    return 1;
}
