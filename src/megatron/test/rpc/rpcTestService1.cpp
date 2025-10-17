#include "rpcTestService1.h"
#include "IUtils.h"
#include "colorOutput.h"
#define BUF_SIZE_MAX (8*100*1)
#define REMOTE_ADDR "127.0.0.1:2001"
#define N_PONG 12000
#define TI_ACTIVITY_VALUE 2.0
//extern bool done_test;
rpcTestService1::rpcTestService1(const SERVICE_id &id, const std::string&  nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
    ssl=(I_ssl*)iUtils->queryIface(Ifaces::SSL);

}


void rpcTestService1::sendRequest(int session,int seq)
{
//    if(done_test)return;
//    size_t sz=rand()%BUF_SIZE_MAX;
    //char s[sz];
    std::string buf;
    buf+="klallosalkjf;";//[i]=i;//rand();
//    while(buf.size()<sz)
//    {
//    }
    //std::string buf(s,sz);
    // std::string md5=ssl->md5(buf);

//    if(seq%1000==0)
//        logErr2("send %d:%d",session,seq+1);
#ifdef USE_LOCAL
    sendEvent(ServiceEnum::rpcTestService2,new testEvent::testREQ(session,seq+1,buf,md5,ListenerBase::serviceId));
#else
    msockaddr_in sa;
    sa.initFromUrl(REMOTE_ADDR);
//        caps.insert(sa);
    sendEvent(sa,
              ServiceEnum::rpcTestService2,new testEvent::testREQ(session,seq+1,buf,"",ListenerBase::serviceId));
#endif

}
bool rpcTestService1::handleEvent(const REF_getter<Event::Base>& e)
{
//    if(done_test)return true;
    auto& ID=e->id;
    if(timerEventEnum::TickAlarm==ID)
    {
        timerEvent::TickAlarm* ee=(timerEvent::TickAlarm*)e.get();
        // if(ee->tid==TI_START)
        // {
        //     sendRequest(session,0);
        //     return true;
        // }
        // if(ee->tid==TI_ACTIVITY)
        // {
        //     session++;
        //     sendRequest(session,0);
        //     return true;
        // }
        return true;
    }
    if(systemEventEnum::startService==ID)
    {
        /// timeout to wait second peer to bind
        // sendEvent(ServiceEnum::Timer,new timerEvent::SetAlarm(TI_START,NULL,NULL,0.3,ListenerBase::serviceId));
        // sendEvent(ServiceEnum::Timer,new timerEvent::SetAlarm(TI_ACTIVITY,NULL,NULL,TI_ACTIVITY_VALUE,ListenerBase::serviceId));
        sendRequest(session,0);
        return true;
    }
    if(rpcEventEnum::IncomingOnConnector==ID)
    {
        rpcEvent::IncomingOnConnector *ioc=(rpcEvent::IncomingOnConnector *)e.get();
        auto &IDC=ioc->e->id;
        if(testEventEnum::testRSP==IDC)
            return on_testRSP((testEvent::testRSP*)ioc->e.get());
    }
    if(testEventEnum::testRSP==ID)
        return on_testRSP((testEvent::testRSP*)e.get());
    return false;
}
bool rpcTestService1::on_testRSP(const testEvent::testRSP* e)
{
//    if(done_test)return true;
//    if(e->seq%1000==0)
//        logErr2("on rsp %d",e->seq);
    // sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(TI_ACTIVITY,NULL,NULL,TI_ACTIVITY_VALUE,ListenerBase::serviceId));

//    if(e->seq>N_PONG)
    {
//        logErr2(GREEN("RPC test OK %d"),e->seq);
//        done_test=true;
//        return true;
    }
    // if(ssl->md5(e->buf)!=e->md5)
    // {
    //     throw CommonError("wrong md5 of buffer");
    // }

    sendRequest(e->session,e->seq);

    return true;

}
