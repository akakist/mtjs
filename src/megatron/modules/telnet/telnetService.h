#pragma once

#include <REF.h>
#include <epoll_socket_info.h>
#include <string>
#include <unknown.h>
#include <listenerBuffered1Thread.h>
#include <broadcaster.h>
#include "event_mt.h"
#include <Events/System/Net/socketEvent.h>
#include <Events/System/Run/startServiceEvent.h>
#include "Events/Tools/telnetEvent.h"

namespace Telnet
{

    class Node;
    class Session:public Refcountable
    {


    public:
        Session(const REF_getter<Node> &N,const REF_getter<epoll_socket_info>& _esi);

        REF_getter<Node> mx_defaultNode;
        std::string mx_current_command_line;
        std::vector<std::string> mx_command_history;
        int curpos;
        int command_history_pos;
        bool insertMode;
        int width,height;
        REF_getter<epoll_socket_info> esi;
        REF_getter<Node> defaultNode()
        {
            return mx_defaultNode;
        }
        void defaultNode(const REF_getter<Node> &n)
        {
            mx_defaultNode=n;
        }

        bool on_TKEY_BS();

    };


    class _Command: public Refcountable
    {
    public:
        const route_t dstService;
        const std::string cmd_re;
        // const std::deque<std::string> params;
        const std::string help;
        _Command(const route_t& _svs, const std::string& _cmd_re,  const std::string& _help)
            : dstService(_svs), cmd_re(_cmd_re),help(_help)
        {}

    };
    class Node: public Refcountable
    {
        std::map<std::string,REF_getter<Node> > m_children;
        std::map<std::string,REF_getter<_Command> > m_commands;
    public:
        Node(const std::string& _name, Node* _parent, const std::string& _help):name(_name), help(_help),parent(_parent) {}
        const std::string name;
        const std::string help;
        Node* const parent;
        REF_getter<Node> getSubDir(const std::string& dir);
        void addChildNode(const std::string& section,const REF_getter<Node>&n);
        void addCommand(const std::string& cmd,const REF_getter<_Command>&n);
        std::map<std::string,REF_getter<Node> > children();
        std::map<std::string,REF_getter<_Command> > commands();
        std::string path();
    };


    class CommandEntries
    {
        REF_getter<Node> m_root;
        // std::map<std::string/*name*/,std::pair<std::string/*pattern*/,std::string/*help*/> > mx_types;
    public:
        CommandEntries(): m_root(new  Node("",NULL,"Top level commands")) {}

        REF_getter<Node> root();
        // void registerType(const std::string& _name, const std::string& _pattern, const std::string& _help);
        void registerDirectory(const std::deque<std::string> &d, const std::string& name, const std::string& help);
        void registerCommand(const std::deque<std::string> &d, const std::string& cmd, const std::string& help,const route_t& dst);
        // std::string getRegex(const std::string &param);
        // std::map<std::string,std::string> getHelpOnTypes();
        // std::map<std::string,std::string> getHelpOnRe();

    };

    class Service:
        public UnknownBase,
        public Broadcaster,
        public ListenerBuffered1Thread
    {
        class __telnet_stuff: public Refcountable//SocketsContainerBase
        {
            std::map<SOCKET_id,REF_getter<Telnet::Session> > sessions;
        public:
            __telnet_stuff()
//                : SocketsContainerBase("__telnet_stuff")
            {}
            void erase(const SOCKET_id& id);
            REF_getter<Telnet::Session> get(const SOCKET_id& id);
            void insert(const SOCKET_id& id,const REF_getter<Telnet::Session> &C);
            std::map<SOCKET_id,REF_getter<Telnet::Session> > getContainer();
            void on_delete(const REF_getter<epoll_socket_info>&esi, const std::string& reason);
            void clear()
            {
                sessions.clear();
            }
        };

        // bool on_RegisterType(const telnetEvent::RegisterType*);
        bool on_RegisterCommand(const telnetEvent::RegisterCommand*);
        bool on_Reply(const telnetEvent::Reply*);

        bool DoListen(const telnetEvent::DoListen*);


        CommandEntries cmdEntries;

        bool on_StreamRead(const socketEvent::StreamRead* evt);
        bool on_Accepted(const socketEvent::Accepted* evt);
        bool on_NotifyBindAddress(const socketEvent::NotifyBindAddress*);
        bool on_Disaccepted(const socketEvent::Disaccepted*);
        bool on_Disconnected(const socketEvent::Disconnected*);

        bool on_startService(const systemEvent::startService*);

        // void doListen();

        void processCommand(const REF_getter<Telnet::Session>&__S, const REF_getter<epoll_socket_info>&esi);
        void prompt(const REF_getter<Telnet::Session>& W, const REF_getter<epoll_socket_info>&esi);
        std::string promptString(const REF_getter<Telnet::Session>& W);

        bool paramMatch(const std::string & re, const std::string & exp);


        // std::set<msockaddr_in> m_bindAddr;
        std::string m_deviceName;
        //bool m_disabled;
    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        static UnknownBase* construct(const SERVICE_id &id, const std::string& nm,IInstance* ifa)
        {
            XTRY;
            return new Service(id,nm,ifa);
            XPASS;
        }
        bool handleEvent(const REF_getter<Event::Base>& e);

        Service(const SERVICE_id& id, const std::string& nm,IInstance* ifa);
        ~Service() {
            stuff->clear();
        }

        REF_getter<__telnet_stuff> stuff;
        IInstance *iInstance;

    };
};
