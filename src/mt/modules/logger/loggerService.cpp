#include "loggerService.h"
#include <stdarg.h>

#include "mutexInspector.h"
#include "events_logger.hpp"
Logger::Service::Service(const SERVICE_id& id, const std::string& nm, IInstance *ifa):
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),Broadcaster(ifa)

{


}

Logger::Service::~Service()
{
    if(log_file!=nullptr)
        fclose(log_file);
}
static std::string getLogName()
{
    std::string fn=(std::string)iUtils->app_name()+".log";
    fn=iUtils->gLogDir()+"/"+fn;
    return fn;
}

bool Logger::Service::on_startService(const systemEvent::startService* e)
{
    auto ln=getLogName();
    log_file=fopen(ln.c_str(),"w");
    if(log_file==nullptr)
        throw CommonError("fopen failed: %s",ln.c_str());
    return true;
}

void registerErrorDispatcherService(const char* pn)
{
    XTRY;
    if(pn)
    {

        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::Logger,"Logger",getEvents_logger());
    }
    else
    {
        iUtils->registerService(ServiceEnum::Logger,Logger::Service::construct,"Logger");
        regEvents_logger();
    }
    XPASS;
}


bool Logger::Service::on_LogMessage(const loggerEvent::LogMessage *e)
{
    MUTEX_INSPECTOR;
    XTRY;

    fprintf(log_file,"%s %s\n",e->opcode.c_str(),e->msg.c_str());
    XPASS;
    return true;
}

bool Logger::Service::on_IncomingOnAcceptor(const rpcEvent::IncomingOnAcceptor*ev)
{
    MUTEX_INSPECTOR;
    XTRY;
    if(!ev) throw CommonError("!ev");
    auto &IDA=ev->e->id;
    if( loggerEventEnum::LogMessage==IDA)
        return on_LogMessage(static_cast<const loggerEvent::LogMessage* > (ev->e.get()));

    logErr2("ErrorDispatcher: unhandled event %s %s %d",iUtils->genum_name(ev->id),__FILE__,__LINE__);
    XPASS;
    return false;
}
bool Logger::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    auto &ID=e->id;


    if( loggerEventEnum::LogMessage==ID)
        return on_LogMessage(static_cast<const loggerEvent::LogMessage* > (e.get()));
    if( systemEventEnum::startService==ID)
        return on_startService(static_cast<const systemEvent::startService*>(e.get()));
    if( rpcEventEnum::IncomingOnAcceptor==ID)
        return(this->on_IncomingOnAcceptor(static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get())));

    logErr2("Logger: unhandled event %s %s %d",iUtils->genum_name(e->id),__FILE__,__LINE__);
    XPASS;
    return false;
}
bool Logger::Service::on_IncomingOnConnector(const rpcEvent::IncomingOnConnector* e)
{

    return true;
}
