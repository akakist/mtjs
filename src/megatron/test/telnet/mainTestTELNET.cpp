#include "IInstance.h"
#include "colorOutput.h"
#include "configObj.h"
#include "CUtils.h"
#include <unistd.h>
#include "commonError.h"
#include "Events/Tools/telnetEvent.h"
void registerSocketModule(const char* pn);
void registerTimerService(const char* pn);
void registerTelnetService(const char* pn);
void registerteldemo1(const char* pn);

int mainTestTELNET(int argc, char** argv )
{
    try {
        iUtils=new CUtils(argc, argv, "telnetTest");

        registerSocketModule(NULL);
        registerTimerService(NULL);
        registerTelnetService(NULL);

        registerteldemo1(NULL);

        printf(GREEN("RUN TEST %s"),__PRETTY_FUNCTION__);

        {
            IInstance *instance1=iUtils->createNewInstance("teldemo1");
            ConfigObj *cnf1=new ConfigObj("teldemo1",
                                          "\nStart=teldemo1"
                                        //   "\nTelnet_BindAddr=127.0.0.1:8081"
                                        //   "\nTelnet_deviceName=Device"
                                          "\nSocketIO_listen_backlog=128"
                                          "\nSocketIO_size=1024"
                                          "\nSocketIO_timeout_millisec=10"

                                          "\nSocketIO_epoll_timeout_millisec=2000"
                                          "\n"
                                          "\n# socket poll thread count"
                                          "\nSocketIO_n_workers=3"
                                          "\nOscar_maxPacketSize=33554432"
                                         );
            instance1->setConfig(cnf1);
            instance1->initServices();
            msockaddr_in sa;
            route_t r;
            sa.init("localhost:8081");
            instance1->sendEvent(ServiceEnum::Telnet,new telnetEvent::DoListen(sa,"Dev1",r));
        }

        printf("connecting to server app with telnet console\nUse help command to get help\n");
        sleep(2);
        auto r=system("telnet 127.0.0.1 8081");
        sleep(1);
        delete iUtils;
        return 0;

    } catch (CommonError& e)
    {
        printf("CommonError %s\n",e.what());
    }
    return 1;
}
