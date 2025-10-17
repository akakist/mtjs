#include "IInstance.h"
#include "colorOutput.h"
#include "configObj.h"
#include "version_mega.h"
#include "rpcTestService1.h"
#include "rpcTestService2.h"
#include "testRpcEvent.h"
#include "CUtils.h"
#include <unistd.h>
//bool done_test=false;
void registerRPCService(const char* pn);
void registerSocketModule(const char* pn);
void registerTimerService(const char* pn);
void registerSSL(const char* pn);
void registerOscarModule(const char* pn);
// void registerOscarSecureModule(const char* pn);
extern std::atomic<int> nreq;

int mainTest(int argc, char** argv )
{
    try {
        iUtils=new CUtils(argc, argv, "rpcTest");
        iUtils->registerService(ServiceEnum::rpcTestService1,rpcTestService1::construct,"rpcTestService1");
        iUtils->registerService(ServiceEnum::rpcTestService2,rpcTestService2::construct,"rpcTestService2");
        iUtils->registerEvent(testEvent::testREQ::construct);
        iUtils->registerEvent(testEvent::testRSP::construct);

        registerRPCService(NULL);
        registerSocketModule(NULL);
        registerTimerService(NULL);
        registerSSL(NULL);
        registerOscarModule(NULL);
        // registerOscarSecureModule(NULL);

        printf(GREEN("RUN TEST %s"),__PRETTY_FUNCTION__);

        {
            IInstance *instance2=iUtils->createNewInstance("rpcTestService2");
            ConfigObj *cnf2=new ConfigObj("rpcTestService2",
                                          "\nStart=RPC,rpcTestService2"
                                          "\nRPC_oscarType=Oscar"
                                          "\nOscarSecure.RSA_keysize=256"
                                          "\nOscarSecure.maxPacketSize=33554432"
                                          "\nRPC_BindAddr_MAIN=INADDR_ANY:2001"
                                          "\nRPC_BindAddr_RESERVE=NONE"
                                          "\nSocketIO_listen_backlog=128"
                                          "\nSocketIO_size=1024"
                                          "\nSocketIO_timeout_millisec=10"
                                          "\nWebHandler.bindAddr=NONE"
                                          "\nOscar_maxPacketSize=33554432"
                                          "\nSocketIO_epoll_timeout_millisec=1000"

                                          "\n# socket poll thread count"
                                          "\nSocketIO_n_workers=2"

                                         );
            instance2->setConfig(cnf2);
            instance2->initServices();
        }

	for(int i=0;i< 10;i++)
        {
            IInstance *instance1=iUtils->createNewInstance("rpcTestService1");
            ConfigObj *cnf1=new ConfigObj("rpcTestService1",
                                          "\nStart=RPC,rpcTestService1"
                                          "\nRPC_oscarType=Oscar"

                                          "\nOscarSecure.RSA_keysize=256"
                                          "\nOscarSecure.maxPacketSize=33554432"
//                                          "\nRPC_BindAddr_MAIN=INADDR_ANY:0"
                                          "\nRPC_BindAddr_RESERVE=NONE"
                                          "\nSocketIO_listen_backlog=128"
                                          "\nSocketIO_size=1024"
                                          "\nSocketIO_timeout_millisec=10"
                                          "\nWebHandler_bindAddr=NONE"
                                          "\nSocketIO_epoll_timeout_millisec=1000"

                                          "\n# socket poll thread count"
                                          "\nSocketIO_n_workers=2"
                                         );
            instance1->setConfig(cnf1);
            instance1->initServices();
        }
	time_t t=time(NULL);
        while(time(NULL)<t+10)
        {
            usleep(100000);
        }
        printf("nreq %d\n",nreq.load());
        iUtils->setTerminate(0);
        delete iUtils;
        return 0;

    } catch (CommonError& e)
    {
        printf("CommonError %s\n",e.what());
    }
    return 1;
}
