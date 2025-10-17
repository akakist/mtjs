#pragma once
#ifdef WEBDUMP
#include <REF.h>
#include <route_t.h>
#include <IUtils.h>
#include <string>
#include <broadcaster.h>
#include <listenerBuffered1Thread.h>
#include "event_mt.h"
#include <Events/System/Run/startServiceEvent.h>
#include <Events/System/Net/httpEvent.h>
#include "Events/Tools/webHandlerEvent.h"
#include <Events/System/Net/httpEvent.h>

namespace WebHandler
{

    struct Node: public Refcountable,public Mutexable
    {
        enum NT
        {
            NT_handler, NT_directory
        };
        int nodeType;
        const std::string urlPart;
        const std::string displayName;
        route_t route;
        REF_getter<WebHandler::Node> parent;

        std::map<std::string, REF_getter<Node> > children;

        Node(const REF_getter<WebHandler::Node>& par,const std::string& _urlPart, const std::string& _displayName, const route_t& _route):
            nodeType(NT_handler),urlPart(_urlPart),displayName(_displayName),route(_route),parent(par) {}
        Node(const REF_getter<WebHandler::Node>& par,const std::string& _urlPart, const std::string& _displayName):
            nodeType(NT_directory),urlPart(_urlPart),displayName(_displayName),parent(par) {}
        virtual ~Node()
        {
        }

        void remove()
        {
            std::map<std::string, REF_getter<Node> >  c;
            {
                M_LOCK(this);
                c=children;
                children.clear();
            }
            while(c.size())
            {
                REF_getter<Node> n=c.begin()->second;
                n->parent=NULL;
                n->remove();
                c.erase(c.begin());
            }
        }
        // nlohmann::json jdump()
        // {
        //     nlohmann::json v;
        //     v["urlPart"]=urlPart;
        //     v["displayName"]=displayName;
        //     if(nodeType==NT_handler)
        //     {
        //         route_t r=route;
        //         r.pop_front();
        //         v["destination"]=r.dump();
        //     }
        //     std::map<std::string, REF_getter<Node> > c;
        //     {
        //         M_LOCK(this);
        //         c=children;
        //     }
        //     for(auto &i:c)
        //     {
        //         v["children"].push_back(i.second->jdump());
        //     }
        //     return v;
        // }
        std::string path()
        {
            M_LOCK(this);
            std::deque<std::string> q;
            REF_getter<WebHandler::Node> n=this;
            while(n.valid())
            {
                q.push_front(n->urlPart);
                n=n->parent;
            }
            return iUtils->join("/",q);
        }
        std::map<std::string, REF_getter<Node> > getChildren()
        {
            M_LOCK(this);
            return children;
        }
        void addChild(const REF_getter<Node> &n)
        {
            M_LOCK(this);
            children.insert(std::make_pair(n->urlPart,n));
        }
        REF_getter<Node> getChild(const std::string& _url)
        {
            M_LOCK(this);
            auto i=children.find(_url);
            if(i!=children.end()) return i->second;
            return NULL;

        }
        void removeChild(const std::string& _url)
        {
            M_LOCK(this);
            auto i=children.find(_url);
            if(i==children.end()) return;
            children.erase(_url);
        }
        REF_getter<Node> getByPath(const std::string& u)
        {
            XTRY;
            std::deque<std::string> v=iUtils->splitStringDQ("/",u);
            REF_getter<Node> n(this);
            for(size_t i=0; i<v.size(); i++)
            {
                n=n->getChild(v[i]);
                if(!n.valid())
                    break;
            }
            return n;
            XPASS;
        }
        REF_getter<Node> getByPathParentDir(const std::string& u)
        {
            XTRY;
            std::deque<std::string> v=iUtils->splitStringDQ("/",u);
            REF_getter<Node> n(this);
            if(v.size())
            {
                v.pop_back();
                for(size_t i=0; i<v.size(); i++)
                {
                    n=n->getChild(v[i]);
                    if(!n.valid())
                    {
                        DBG(logErr2("cannot find subdir %s",v[i].c_str()));
                        return NULL;

                    }
                }

            }
            return n;
            XPASS;
        }

    };

    class Service:
        public UnknownBase,
        public Broadcaster,
        public ListenerBuffered1Thread
    {

        // config
        REF_getter<WebHandler::Node> root;
        std::set<msockaddr_in> m_bindAddr;

        //!config



        bool on_RequestIncoming(const httpEvent::RequestIncoming*);
        bool on_RegisterHandler(const webHandlerEvent::RegisterHandler*);
        bool on_RegisterDirectory(const webHandlerEvent::RegisterDirectory*);
        bool on_GetBindPortsRSP(const httpEvent::GetBindPortsRSP*) {
            return false;
        }

        IInstance *iInstance;
    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        static UnknownBase*construct(const SERVICE_id&id, const std::string& nm,IInstance* ifa);
        Service(const SERVICE_id& id, const std::string& nm, IInstance *ifa);
        virtual ~Service();

    protected:
        bool handleEvent(const REF_getter<Event::Base>& e);
        bool on_startService(const systemEvent::startService*);
    };
};
#endif
