#include "mutexInspector.h"
#include <time.h>
#include <time.h>
#include <map>
#include "teldemo1Service.h"
#include "events_teldemo1.hpp"


const char *calcs=R"(^([0-9\.]+)([\+\-\*\/])([0-9\.]+)$)";

bool teldemo1::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;

    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("demo1","^push_back\\s+(.+)$","push data into queue back",ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("demo1","^push_front\\s+(.+)$","push data into queue front",ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("demo1","^pop_front$","pop data from front of queue",ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("demo1","^pop_back$","pop data from back of queue",ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("demo1","^front$","get first element from queue",ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("demo1","^back$","get last element from queue",ListenerBase::serviceId));
    sendEvent(ServiceEnum::Telnet,new telnetEvent::RegisterCommand("demo1",calcs,"calc",ListenerBase::serviceId));
    return true;
}
#include <regex>
bool match(const std::string & re, const std::string& buf, std::vector<std::string> &tokens)
{
    MUTEX_INSPECTOR;
    std::regex rgx(re);
    std::smatch match;
    if(std::regex_search(buf,match,rgx))
    {
        tokens.clear();
        for(size_t i=0; i<match.size(); i++)
        {
            tokens.push_back(match[i].str());
        }
        return true;
    }
    return false;
}

bool  teldemo1::Service::on_CommandEntered(const telnetEvent::CommandEntered *e)
{

    std::vector<std::string> tokens;

    if(match(calcs, e->command, tokens))
    {
        if(tokens.size()<4)
        {
            sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"Usage: calc <expression>",ListenerBase::serviceId));
            return true;
        }
        try
        {
            // double result = iUtils->calc(tokens[1]);
            double left = std::stod(tokens[1]);
            double right = std::stod(tokens[3]);
            double result = 0.0;
            if(tokens[2]=="+")
                result = left + right;
            else if(tokens[2]=="-")
                result = left - right;
            else if(tokens[2]=="*")
                result = left * right;
            else if(tokens[2]=="/")
            {
                if(right == 0.0)
                {
                    sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"Error: Division by zero",ListenerBase::serviceId));
                    return true;
                }
                result = left / right;
            }
            sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,std::to_string(result),ListenerBase::serviceId));
        }
        catch(const std::exception &ex)
        {
            sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,std::string("Error: ")+ex.what(),ListenerBase::serviceId));
        }
        return true;
    }
    if(match("^push_back\\s+(.+)$", e->command, tokens))
    {   
        if(tokens.size()<2)
        {
            sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"Usage: push_back <data>",ListenerBase::serviceId));
            return true;
        }
        sampleDeque.push_back(tokens[1]);
        sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"OK",ListenerBase::serviceId));
        return true;
    }
    if(match("^push_front\\s+(.+)$", e->command, tokens))
    {   
        if(tokens.size()<2)
        {
            sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"Usage: push_back <data>",ListenerBase::serviceId));
            return true;
        }
        sampleDeque.push_front(tokens[1]);
        sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"OK",ListenerBase::serviceId));
        return true;
    }
    if(match("^pop_front$", e->command, tokens))
    {   
        sampleDeque.pop_front();
        sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"OK",ListenerBase::serviceId));
        return true;
    }
    if(match("^pop_back$", e->command, tokens))
    {   
        sampleDeque.pop_back();
        sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,"OK",ListenerBase::serviceId));
        return true;
    }
    if(match("^back$", e->command, tokens))
    {   
        sampleDeque.pop_back();
        sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,sampleDeque.back(),ListenerBase::serviceId));
        return true;
    }
    if(match("^front$", e->command, tokens))
    {   
        sampleDeque.pop_back();
        sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(e->socketId,sampleDeque.front(),ListenerBase::serviceId));
        return true;
    }
    return true;
}

bool teldemo1::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        if(timerEventEnum::TickTimer==ID)
        {
            const timerEvent::TickTimer*ev=static_cast<const timerEvent::TickTimer*>(e.get());
            return true;
        }
        if(telnetEventEnum::CommandEntered==ID)
            return on_CommandEntered((const telnetEvent::CommandEntered*)e.get());
        if(systemEventEnum::startService==ID)
            return on_startService((const systemEvent::startService*)e.get());



        if(rpcEventEnum::IncomingOnConnector==ID)
        {
            rpcEvent::IncomingOnConnector *E=(rpcEvent::IncomingOnConnector *) e.get();
            auto& IDC=E->e->id;


            return false;
        }

        if(rpcEventEnum::IncomingOnAcceptor==ID)
        {
            rpcEvent::IncomingOnAcceptor *E=(rpcEvent::IncomingOnAcceptor *) e.get();
            auto& IDA=E->e->id;

            return false;
        }


    }
    catch(const CommonError& e)
    {
        logErr2("teldemo1WebServer CommonError  %s",e.what());

    }
    catch(const std::exception &e)
    {
        logErr2("teldemo1WebServer std::exception  %s",e.what());
    }

    logErr2("teldemo1: unhandled event %s",iUtils->genum_name(e->id));
    XPASS;
    return false;
}

teldemo1::Service::~Service()
{
}


teldemo1::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins):
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{

}

void registerteldemo1(const char* pn)
{
    MUTEX_INSPECTOR;

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::teldemo1,"teldemo1",getEvents_teldemo1());
    }
    else
    {
        iUtils->registerService(ServiceEnum::teldemo1,teldemo1::Service::construct,"teldemo1");
        regEvents_teldemo1();
    }
    XPASS;
}






static int cnt2;
