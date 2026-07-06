#include "IInstance.h"
#include "colorOutput.h"
#include "configObj.h"
#include "CUtils.h"
#include <unistd.h>
#include <thread>
#include "commonError.h"
#include "Events/System/Net/rpcEvent.h"
void registerRPCService(const char* pn);
void registerSocketModule(const char* pn);
void registerTimerService(const char* pn);
void registerSSL(const char* pn);
void registerOscarModule(const char* pn);
// void registerOscarSecureModule(const char* pn);
//void registertestServerWebService(const char* pn);
void registerHTTPModule(const char* pn);
void registerprodtestServerService(const char* pn);
void registerprodtestServerWebService(const char* pn);
#ifdef WEBDUMP
void registerWebHandlerModule(const char* pn);
#endif

int mainTestHTTP_RPC(int argc, char** argv )
{
    try {
        iUtils=new CUtils(argc, argv, "HTTP_RPC_test");

        registerRPCService(NULL);
        registerSocketModule(NULL);
        registerTimerService(NULL);
        registerSSL(NULL);
        registerOscarModule(NULL);
        // registerOscarSecureModule(NULL);
//        registertestServerWebService(NULL);
        registerHTTPModule(NULL);
        registerprodtestServerService(NULL);
        registerprodtestServerWebService(NULL);
#ifdef WEBDUMP
        registerWebHandlerModule(NULL);
#endif


        printf(GREEN("RUN TEST %s"),__PRETTY_FUNCTION__);

        {
            IInstance *instance1=iUtils->createNewInstance("prodtestServer");
            ConfigObj *cnf1=new ConfigObj("prodtestServer",
                                          "\nStart=prodtestServer,RPC"
                                          "\nOscarSecure.RSA_keysize=256"
                                          "\nOscarSecure.maxPacketSize=33554432"
                                          "\nRPC_BindAddr_MAIN=127.0.0.1:2000"
    "\n#unix@/tmp/sock"
                                          "\nRPC_BindAddr_RESERVE=NONE"
                                          "\nRPC_oscarType=Oscar"
                                          "\nSocketIO_listen_backlog=128"
                                          "\nSocketIO_size=1024"
                                          "\nSocketIO_timeout_millisec=10"
                                          "\nWebHandler.bindAddr=INADDR_ANY:6002"

                                          "\n# http listen address"
                                          "\nHTTP_max_post=1000000"
                                          "\nHTTP_doc_urls=/pics,/html,/css"
                                          "\nHTTP_document_root=./www"
                                          "\nSocketIO_epoll_timeout_millisec=2000"
                                          "\n"
                                          "\n# socket poll thread count"
                                          "\nSocketIO_n_workers=2"
                                          "\nOscar_maxPacketSize=33554432"
                                         );
            instance1->setConfig(cnf1);
            instance1->initServices();
            msockaddr_in sa;
            sa.init("127.0.0.1:2000");
            SECURE sec;
            instance1->sendEvent(ServiceEnum::RPC,new rpcEvent::DoListen(sa,sec));
        }

        {
            IInstance *instance1=iUtils->createNewInstance("prodtestServerWeb");
            ConfigObj *cnf1=new ConfigObj("prodtestServerWeb",
                                          "\nStart=prodtestServerWeb,RPC"
                                          "\nOscarSecure.RSA_keysize=256"
                                          "\nOscarSecure.maxPacketSize=33554432"
                                          "\nRPC_BindAddr_MAIN=INADDR_ANY:0"
                                          "\nRPC_BindAddr_RESERVE=NONE"
                                          "\nRPC_oscarType=Oscar"
                                          "\nSocketIO_listen_backlog=128"
                                          "\nSocketIO_size=1024"
                                          "\nSocketIO_timeout_millisec=10"
                                          "\nWebHandler.bindAddr=INADDR_ANY:6001"
                                          "\n# http listen address"
                                          "\ntestHTTP_bindAddr=0.0.0.0:8188"
                                          "\nHTTP_max_post=1000000"
                                          "\nHTTP_doc_urls=/pics,/html,/css"
                                          "\nHTTP_document_root=./www"
                                          "\nSocketIO_epoll_timeout_millisec=2000"
                                          "\n"
                                          "\n# socket poll thread count"
                                          "\nSocketIO_n_workers=20"

                                          "\nOscar_maxPacketSize=33554432"
                                          "\n"
                                          "\n# http listen address:"
                                          "\nprodtestServerWeb_bindAddr=0.0.0.0:8188"
                                          "\n"
                                          "\n# server prodtest address"
                                          "\nprodtestServerWeb_prodtestServerAddr=127.0.0.1:2000"
"\n#unix@/tmp/sock"
                                         );
            instance1->setConfig(cnf1);
            instance1->initServices();
        }

        sleep(2);

        std::vector<std::thread> ths;
//        ths.resize(4);
        for(int i=0;i<1;i++)
        {
    	ths.push_back(std::thread([]{
            auto r=system("ab -n 10000000  -k -c 200  http://127.0.0.1:8188/");
    	    }));
        }
        for(auto& z: ths)
        {
    	    z.join();
        }


        // auto ef=system("ab -n 10000000 -k -c 200  http://127.0.0.1:8188/");
        // if(ef!=0)
            // sleep(60*60);
            sleep(100);
        iUtils->setTerminate(0);
        sleep(1);
        delete iUtils;
        return 0;

    } catch (CommonError& e)
    {
        printf("CommonError %s\n",e.what());
    }
    return 1;
}
