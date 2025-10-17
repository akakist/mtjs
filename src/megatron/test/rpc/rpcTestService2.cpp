#include "rpcTestService2.h"
#include "tools_mt.h"
//extern bool done_test;
rpcTestService2::rpcTestService2(const SERVICE_id &id, const std::string&  nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
    ssl=(I_ssl*)iUtils->queryIface(Ifaces::SSL);
}
std::atomic<int> nreq=0;
bool rpcTestService2::on_testREQ(const testEvent::testREQ* e)
{
    nreq++;
//    if(done_test)return true;
//    if(e->seq%1000==0)
//        logErr2("recv %d",e->seq);
    int n=nreq;
    if(n%1000000==0)
	printf("nreq %d\n",n);
    // if(ssl->md5(e->buf)!=e->md5)
    // {
    //     throw CommonError("wrong md5 of buffer");
    // }
    passEvent(new testEvent::testRSP(e->session,e->seq,e->buf,e->md5,poppedFrontRoute(e->route)));
    return true;

}
bool rpcTestService2::handleEvent(const REF_getter<Event::Base>& e)
{
//    if(done_test)return true;
    auto& ID=e->id;
    if(systemEventEnum::startService==ID)
    {
        return true;
    }
    if(rpcEventEnum::IncomingOnAcceptor==ID)
    {
        rpcEvent::IncomingOnAcceptor *ioa=(rpcEvent::IncomingOnAcceptor *)e.get();
        auto &IDA=ioa->e->id;
        if(testEventEnum::testREQ==IDA)
            return on_testREQ((testEvent::testREQ*)ioa->e.get());

    }
    if(testEventEnum::testREQ==ID)
    {
        testEvent::testREQ* ee=(testEvent::testREQ*) e.get();
        // if(ssl->md5(ee->buf)!=ee->md5)
        // {
        //     throw CommonError("wrong md5 of buffer");
        // }
        passEvent(new testEvent::testRSP(ee->session,ee->seq,ee->buf,ee->md5,poppedFrontRoute(e->route)));
        return true;
    }
    return false;
}
