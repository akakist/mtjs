#include "listenerBuffered1Thread.h"
#include "ISSL.h"
#include "broadcaster.h"
#include "testRpcEvent.h"
#include "msockaddr_in.h"
#include "Events/System/timerEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/Net/rpcEvent.h"




class rpcTestService2: public UnknownBase, public ListenerBuffered1Thread, public Broadcaster
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
        return new rpcTestService2(id,nm,ifa);
        XPASS;
    }
    rpcTestService2(const SERVICE_id &id, const std::string&  nm,IInstance* ins);
    bool on_testREQ(const testEvent::testREQ* e);

    bool handleEvent(const REF_getter<Event::Base>& e);
};
