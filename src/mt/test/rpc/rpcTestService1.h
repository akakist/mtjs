#include "listenerBuffered1Thread.h"
#include "ISSL.h"
#include "broadcaster.h"
#include "testRpcEvent.h"
#include "msockaddr_in.h"
#include "Events/System/timerEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/Net/rpcEvent.h"

//extern bool done_test;
static int session=0;
enum timers
{
    TI_START,
    TI_ACTIVITY
};

class rpcTestService1: public UnknownBase, public ListenerBuffered1Thread, public Broadcaster
{
public:
    I_ssl *ssl;
    void deinit()
    {
        ListenerBuffered1Thread::deinit();
    }

    static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
    {
        XTRY;
        return new rpcTestService1(id,nm,ifa);
        XPASS;
    }
    rpcTestService1(const SERVICE_id &id, const std::string&  nm,IInstance* ins);
    void sendRequest(int session,int seq);
    bool on_testRSP(const testEvent::testRSP* e);
    bool handleEvent(const REF_getter<Event::Base>& e);
};
