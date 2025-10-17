#include "IInstance.h"
#include "colorOutput.h"
#include "configObj.h"
#include "CUtils.h"
#include <unistd.h>
#include "commonError.h"
//bool done_test_http=false;
void registerRPCService(const char* pn);
void registerSocketModule(const char* pn);
void registerTimerService(const char* pn);
void registerSSL(const char* pn);
void registerOscarModule(const char* pn);
// void registerOscarSecureModule(const char* pn);
void registerDFSReferrerService(const char* pn);
void registerDFSCapsService(const char* pn);
void registerGEOIP(const char* pn);
int mainTestReferrer(int argc, char** argv )
{
    try {
        iUtils=new CUtils(argc, argv, "referrerTest");

        registerRPCService(NULL);
        registerSocketModule(NULL);
        registerTimerService(NULL);
        registerSSL(NULL);
        registerOscarModule(NULL);
        // registerOscarSecureModule(NULL);
        registerDFSReferrerService(NULL);
        registerDFSCapsService(NULL);
        registerGEOIP(NULL);

        printf(GREEN("RUN TEST %s"),__PRETTY_FUNCTION__);

        {
            const char *cf=R"zxc(

# list of initially started services
Start=RPC,DFSCaps,DFSReferrer
RPC_oscarType=Oscar

# Address used to work with dfs network. NONE - no bind
RPC_BindAddr_MAIN=INADDR_ANY:5500,INADDR6_ANY:0

# Address used to communicate with local apps, must be fixed. NONE - no bind
RPC_BindAddr_RESERVE=NONE
SocketIO_epoll_timeout_millisec=1000
SocketIO_listen_backlog=128

# socket poll thread count
SocketIO_n_workers=2

# Is caps mode. Do not forward caps requests to uplink
DFSReferrer_IsCapsServer=true
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

)zxc";
            IInstance *instance1=iUtils->createNewInstance("root");
            ConfigObj *cnf1=new ConfigObj("root.conf",cf);
            instance1->setConfig(cnf1);
            instance1->initServices();
            sleep(1);
        }
        for(int i=0; i<2; i++)
        {

            const char *cf=R"zxc(

# list of initially started services
Start=RPC,DFSReferrer

# Address used to work with dfs network. NONE - no bind
RPC_BindAddr_MAIN=INADDR_ANY:0,INADDR6_ANY:0

# Address used to communicate with local apps, must be fixed. NONE - no bind
RPC_BindAddr_RESERVE=NONE
SocketIO_epoll_timeout_millisec=1000
SocketIO_listen_backlog=128

# socket poll thread count
SocketIO_n_workers=2

# Is caps mode. Do not forward caps requests to uplink
DFSReferrer_IsCapsServer=false
DFSReferrer_CapsServerUrl=127.0.0.1:5500
DFSReferrer_T_001_common_connect_failed=20.000000
DFSReferrer_T_002_D3_caps_get_service_request_is_timed_out=15.000000
DFSReferrer_T_007_D6_resend_ping_caps_short=7.000000
DFSReferrer_T_008_D6_resend_ping_caps_long=20.000000
DFSReferrer_T_009_pong_timed_out_caps_long=40.000000
DFSReferrer_T_011_downlink_ping_timed_out=60.000000
DFSReferrer_T_012_reregister_referrer=3500.000000
DFSReferrer_T_020_D3_1_wait_after_send_PT_CACHE_on_recvd_from_GetService=2.000000
DFSReferrer_T_004_cache_pong_timed_out_=2.000000


)zxc";
            IInstance *instance1=iUtils->createNewInstance("node_"+std::to_string(i));

            ConfigObj *cnf1=new ConfigObj("node.conf",cf);
            instance1->setConfig(cnf1);
            instance1->initServices();
            sleep(1);

        }
        while(!iUtils->isTerminating())
        {
            sleep(1);
        }
        delete iUtils;
        return 0;

    } catch (const CommonError& e)
    {
        printf("CommonError %s\n",e.what());
    }
    return 1;
}
