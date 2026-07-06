#ifdef WEBDUMP
#include <stdlib.h>

#ifndef _WIN32
#endif
#include "webHandlerService.h"

#include <Events/System/Net/httpEvent.h>
#include "Events/Tools/webHandlerEvent.h"

#include "mutexInspector.h"
#include "version_mega.h"
#include "events_webHandler.hpp"

UnknownBase* WebHandler::Service::construct(const SERVICE_id &id, const std::string& nm, IInstance* ifa)
{
    XTRY;
    return new Service(id,nm,ifa);
    XPASS;
}
WebHandler::Service::~Service()
{
    root->remove();
}

WebHandler::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ifa):
    UnknownBase(nm),
    Broadcaster(ifa),
    ListenerBuffered1Thread(nm,id),
    root(new WebHandler::Node(NULL,"","Home")),iInstance(ifa)
{
    XTRY;
    m_bindAddr=ifa->getConfig()->get_tcpaddr("bindAddr","NONE","Addr:port to bind. Addr=INADDR_ANY:port - bind to all interfaces. NONE - no bind");
    XPASS;
}


bool WebHandler::Service::on_startService(const systemEvent::startService *)
{
    if(root.valid())
    {
        root->remove();
    }
    for(auto &i:m_bindAddr)
    {
        SECURE sec;
        sec.use_ssl=false;
        sendEvent(ServiceEnum::HTTP,new httpEvent::DoListen(i,sec, this));
    }


    return true;
}
bool WebHandler::Service::on_RegisterHandler(const webHandlerEvent::RegisterHandler* e)
{
    std::vector<std::string> v=iUtils->splitString("/",e->url);
    REF_getter<WebHandler::Node> n=root->getByPathParentDir(e->url);
    if(!n.valid()) throw CommonError("base dir for url %s not defined",e->url.c_str());
//    DBG(logErr2("on_RegisterHandler parent %s node %s",n->path().c_str(),e->url.c_str()));
    n->addChild(new WebHandler::Node(n,v[v.size()-1],e->displayName,e->route));
    return true;
}

bool WebHandler::Service::on_RegisterDirectory(const webHandlerEvent::RegisterDirectory* e)
{
    std::vector<std::string> v=iUtils->splitString("/",e->url);
    REF_getter<WebHandler::Node> n=root->getByPathParentDir(e->url);
    if(!n.valid()) throw CommonError("base dir for url %s not defined",e->url.c_str());
//    DBG(logErr2("on_RegisterDirectory parent %s node %s",n->path().c_str(),e->url.c_str()));
    n->addChild(new WebHandler::Node(n,v[v.size()-1],e->displayName));
    return true;
}
#include "httpUrlParser.h"
bool WebHandler::Service::on_RequestIncoming(const httpEvent::RequestIncoming* e)
{
    URIParser parser(e->req->uri());

    // DBG(logErr2("%s",e->req->url.c_str()));
    if(parser.path()=="/webdump")
    {
        std::string ps=iUtils->hex2bin((std::string)parser.params_["p"]);
        WebDumpable *d;
        if(sizeof(d)!=ps.size()) throw CommonError("sizeof(d)!=ps.size()");
        memcpy(&d,ps.data(),ps.size());
        std::string out=iUtils->dumpWebDumpable(d);
        HTTP::Response cc(e->req);
        cc.make_response(out);
        return true;
    }

    REF_getter<WebHandler::Node> N=root->getByPath((std::string)parser.path());
    if(!N.valid())
    {
        DBG(logErr2("!N valid"));
        HTTP::Response cc(e->req);
        {
            std::string out;
            std::string url;
            {
                url=parser.path();
            }
            {
                out.append("url "+url+" Not found");
            }
            {
                cc.make_response(out);
            }
        }
        return true;
    }
    if(N->nodeType==WebHandler::Node::NT_handler)
    {
        DBG(logErr2("N->nodeType==WebHandler::Node::NT_handler"));
        route_t r=N->route;
        r.pop_front();
        DBG(logErr2("route %s",r.dump().c_str()));
        DBG(logErr2("WebHandler::Node::NT_handler route %s",r.dump().c_str()));
        passEvent(new webHandlerEvent::RequestIncoming(e->req,e->esi,r));
    }
    else if(N->nodeType==WebHandler::Node::NT_directory)
    {
        DBG(logErr2("N->nodeType==WebHandler::Node::NT_directory"));
        HTTP::Response cc(e->req);
        std::string out="<h1>"+N->displayName+"</h1><p>";
        std::map<std::string, REF_getter<Node> > c=N->getChildren();
        for(auto &i:c)
        {
            out+="<a href='"+i.second->path()+"'>"+i.second->displayName+"</a><br>";
        }
        cc.make_response(out);
        return true;
    }
    else
    {
        throw CommonError("invalid node type");
    }
    return true;
}


bool WebHandler::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    auto &ID=e->id;
    if( webHandlerEventEnum::RegisterHandler==ID)
        return(this->on_RegisterHandler((const webHandlerEvent::RegisterHandler*)e.get()));
    if( webHandlerEventEnum::RegisterDirectory==ID)
        return(this->on_RegisterDirectory((const webHandlerEvent::RegisterDirectory*)e.get()));
    if( httpEventEnum::RequestIncoming==ID)
        return(this->on_RequestIncoming((const httpEvent::RequestIncoming*)e.get()));
    if( httpEventEnum::GetBindPortsRSP==ID)
        return(this->on_GetBindPortsRSP((const httpEvent::GetBindPortsRSP*)e.get()));
    if(systemEventEnum::startService==ID)
        return on_startService((const systemEvent::startService*)e.get());

    XPASS;
    return false;
}


void registerWebHandlerModule(const char* pn)
{
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::WebHandler,"WebHandler",getEvents_webHandler());
    }
    else
    {
        iUtils->registerService(ServiceEnum::WebHandler,WebHandler::Service::construct,"WebHandler");
        regEvents_webHandler();
    }
}
#endif
