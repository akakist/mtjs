#include "IInstance.h"
#include "colorOutput.h"
#include "configObj.h"
#include "CUtils.h"
#include <unistd.h>
#include "commonError.h"
void registerTestTimer(const char* pn);
void registerTimerService(const char* pn);

int mainTestTimer(int argc, char** argv )
{
    try {
        iUtils=new CUtils(argc, argv, "timerTest");

        registerTimerService(NULL);
        registerTestTimer(NULL);

        printf(GREEN("RUN TEST %s"),__PRETTY_FUNCTION__);

        {
            IInstance *instance1=iUtils->createNewInstance("testtimer1");
            ConfigObj *cnf1=new ConfigObj("testtimer1",
                                          "\nStart=testTimer"
                                         );
            instance1->setConfig(cnf1);
            instance1->initServices();
        }
        while(!iUtils->isTerminating())
        {
            sleep(1);
        }
        auto ef=iUtils->getExitFlag();

        delete iUtils;
        return ef;

    } catch (CommonError& e)
    {
        printf("CommonError %s\n",e.what());
    }
    return 1;
}
