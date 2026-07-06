#include "IInstance.h"
#include "colorOutput.h"
#include "configObj.h"
#include "CUtils.h"
#include <unistd.h>
#include <thread>
#include "commonError.h"
//bool done_test_http=false;
void registerRPCService(const char* pn);
void registerSocketModule(const char* pn);
void registerTimerService(const char* pn);
void registerSSL(const char* pn);
void registerOscarModule(const char* pn);
// void registerOscarSecureModule(const char* pn);
// void registertestServerWebService(const char* pn);
void registerHTTPModule(const char* pn);
void registerTestServerWebService2(const char* pn);
void registerTestWSCService(const char* pn);
void registerWebSocketClientService(const char* pn);


int mainTestWSC(int argc, char** argv )
{
    try {
        iUtils=new CUtils(argc, argv, "wsTest");

        registerRPCService(NULL);
        registerSocketModule(NULL);
        registerTimerService(NULL);
        registerSSL(NULL);
        registerOscarModule(NULL);
        // registerOscarSecureModule(NULL);
        // registertestServerWebService(NULL);
        registerHTTPModule(NULL);
        registerWebSocketClientService(NULL);
        registerTestServerWebService2(NULL);
        registerTestWSCService(NULL);

        printf(GREEN("RUN TEST %s"),__PRETTY_FUNCTION__);

        {
            IInstance *instance1=iUtils->createNewInstance("testWS1");
            ConfigObj *cnf1=new ConfigObj("testHTTP1",
                                          "\nStart=testWSC,testHTTP2"
                                          "\ntestWSC_url=ws://localhost:8087/zaza"
                                          "\ntestHTTP2_bindAddr=0.0.0.0:8087"
                                          "\nSocketIO_listen_backlog=128"
                                          "\nSocketIO_size=1024"
                                          "\nSocketIO_timeout_millisec=7000"
                                          "\nWebHandler_bindAddr=NONE"

                                          "\n# http listen address"
                                          "\nHTTP_max_post=1000000"
                                          "\nHTTP_doc_urls=/pics,/html,/css"
                                          "\nHTTP_document_root=./www"
                                          "\nSocketIO_epoll_timeout_millisec=12000"
                                          "\n"
                                          "\n# socket poll thread count"
                                          "\nSocketIO_n_workers=20"
                                         );
            instance1->setConfig(cnf1);
            instance1->initServices();
        }

        usleep(1000000);
        std::vector<std::thread> ths;
//        ths.resize(4);
        for(int i=0;i<4;i++)
        {
    	ths.push_back(std::thread([i]{
            logErr2("run thr %d",i);
            // system("ab -n 10000000  -k -c 100  http://127.0.0.1:8188/");
    	    }));
        }
        for(auto& z: ths)
        {
    	    z.join();
        }
        while(1)        
        sleep(1);
//        auto ef=system("ab -n 10000000  -k -c 100  http://127.0.0.1:8188/");
//        auto ef=system("wrk  -c500 -d30s -t100 http://127.0.0.1:8188/");
        iUtils->setTerminate(0);
//        system("ab -n 1000000   -c 10  http://127.0.0.1:8088/");
//        sleep(1);
        delete iUtils;
        return 0;

    } catch (CommonError& e)
    {
        printf("CommonError %s\n",e.what());
    }
    return 1;
}
