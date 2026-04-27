#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <time.h>
#include <map>
#include "broadcasterTreeService.h"
#include "tools_mt.h"
#include "tree.h"
#include "events_broadcasterTreeService.hpp"
#include "version_mega.h"
#include "tr_exec.h"
#include "CDatabase.h"
#include "s_ed.h"
#include "QUORUM.h"
#include <SQLiteCpp/Database.h>
#include "CDatabase.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "init_root.h"



bool BroadcasterTree::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;

    return true;
}

bool BroadcasterTree::Service::on_timer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool BroadcasterTree::Service::on_alarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    switch(e->tid)
    {
    case TIMER_BROADCAST_ACK_TIMEDOUT:
    {
        MUTEX_INSPECTOR;
        bcEvent::SendToChild *c=dynamic_cast<bcEvent::SendToChild*>(e->cookie.get());
        if(!c) throw CommonError("if(!c) 1222447");

        logErr2("TIMER_BROADCAST_ACK_TIMEDOUT %s",c->dstNodeName.container.c_str());

        make_broadcast_message_to_tree(c->dst_service, c->payload,c->bt, c->route);

        return true;

    }
    break;

    }
    return false;

    return false;
}


bool BroadcasterTree::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        switch(ID)
        {
        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot*)e.get());
        case bcEventEnum::BroadcastMessage:
            return BroadcastMessage((const bcEvent::BroadcastMessage*)e.get());
        // case bcEventEnum::GetTransactions:
        //     return GetTransactions((const bcEvent::GetTransactions*)e.get());
        // case bcEventEnum::ClientMsg:
        //     return ClientMsg((const bcEvent::ClientMsg*)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit*)e.get());
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer*)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm*)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService*)e.get());
        case bcEventEnum::MsgReply:
            return MsgReply(static_cast<const bcEvent::MsgReply*>(e.get()),false);
        case bcEventEnum::SendToChild:
            return SendToChild(static_cast<const bcEvent::SendToChild*>(e.get()),false);
        case bcEventEnum::SendToChildAck:
            return SendToChildAck(static_cast<const bcEvent::SendToChildAck*>(e.get()),false);

        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor*ev=static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get());
            auto &IDA=ev->e->id;

            switch(IDA)
            {
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply*>(ev->e.get()),true);
            case bcEventEnum::SendToChild:
                return SendToChild(static_cast<const bcEvent::SendToChild*>(ev->e.get()),true);
            case bcEventEnum::SendToChildAck:
                return SendToChildAck(static_cast<const bcEvent::SendToChildAck*>(ev->e.get()),true);

            default:
                throw CommonError("unhabdled ev %d %s",IDA, iUtils->genum_name(IDA));
            }
        }
        break;
        case rpcEventEnum::IncomingOnConnector:
        {
            const rpcEvent::IncomingOnConnector*ev=static_cast<const rpcEvent::IncomingOnConnector*>(e.get());
            auto &IDC=ev->e->id;
            switch(IDC)
            {
            case bcEventEnum::MsgReply:
                return MsgReply(static_cast<const bcEvent::MsgReply*>(ev->e.get()),true);
            case bcEventEnum::SendToChild:
                return SendToChild(static_cast<const bcEvent::SendToChild*>(ev->e.get()),true);
            case bcEventEnum::SendToChildAck:
                return SendToChildAck(static_cast<const bcEvent::SendToChildAck*>(ev->e.get()),true);

            default:
                throw CommonError("unhabdled ev %d %s",IDC, iUtils->genum_name(IDC));
            }
        }
        break;

        default:
            throw CommonError("unhabdled ev %d %s",ID, iUtils->genum_name(ID));
        }



    } catch(std::exception &e)
    {
        logErr2("BroadcasterTree std::exception  %s",e.what());
    }
    XPASS;
    return false;
}
#include <regex>

BroadcasterTree::Service::~Service()
{
}


BroadcasterTree::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
}

bool BroadcasterTree::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf=e;
    if(!root.valid())
        root=getRoot(conf->db.get());

    init_root(root);
    return true;
}
#ifdef KALL
bool BroadcasterTree::Service::GetTransactions(const bcEvent::GetTransactions*e)
{
    msg::response_with_transactions rwt;
    for(auto& z: transaction_pool_verified)
    {
        rwt.trs.push_back(z.second);
    }
    transaction_pool_verified.clear();
    msg::node_message_ed nm(rwt.getBuffer(),conf->this_node_name,conf->my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(e->route)));

    return true;
}
#endif
bool BroadcasterTree::Service::InvalidateRoot(const bcEvent::InvalidateRoot*e)
{
    root=getRoot(conf->db.get());
    init_root(root);
    return true;
}
bool BroadcasterTree::Service::BroadcastMessage(const bcEvent::BroadcastMessage*e)
{
    MUTEX_INSPECTOR;
    std::map<NODE_id,BroadcasterTree::Node> nodes;

    auto ks=root->getNodesNames(NULL);
    for(auto &nd: ks)
    {
        auto nn=root->getNode(nd,NULL);
        if(nn->disabled_manual || nn->disabled_offline)
            continue;
        BroadcasterTree::Node n;
        n.name=nn->name;
        n.stake=nn->total_stake;
        n.ip=nn->ip;
        nodes[nn->name]=n;
    }
    if(nodes.size()==0)
        return true;
    BroadcasterTree::TreeNode root = BroadcasterTree::buildTree(nodes, conf->this_node_name);

    make_broadcast_message_to_tree(e->dstService, e->msg,root, e->route);
    return true;    
}
void BroadcasterTree::Service::make_broadcast_message_to_tree(SERVICE_id dstService, const std::string & msg, const BroadcasterTree::TreeNode& root, const route_t& route)
{
    MUTEX_INSPECTOR;
    auto &ch=root.children;
    for(auto it=ch.begin(); it!=ch.end(); it++)
    {
        MUTEX_INSPECTOR;
        REF_getter<bcEvent::SendToChild> e1=new bcEvent::SendToChild(msg,*it,dstService,it->node.name,route);
        REF_getter<bcEvent::SendToChild> e2=new bcEvent::SendToChild(msg,*it,dstService,it->node.name,route);
        sendEvent(it->node.ip, ServiceEnum::BroadcasterTree, e1.get());

        sendEvent(ServiceEnum::Timer,new timerEvent::SetAlarm(TIMER_BROADCAST_ACK_TIMEDOUT,
                  toRef(e2->hash()), e2.get(), BROADCAST_ACK_TIMEDOUT_SEC, this));

    }

}


bool BroadcasterTree::Service::MsgReply(const bcEvent::MsgReply* e, bool fromNetwork)
{
    if(e->route.size())
    {
        passEvent(e);
        return true;
    }
    else throw CommonError("if(e->route.size())");
    return true;
}
void BroadcasterTree::Service::logNode(const char* fmt, ...)
{

    {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stdout,"%ld [BroadcasterTree] [%s] ", time(NULL), 
        conf->this_node_name.container.c_str());
        vfprintf(stdout,fmt, ap);
        fprintf(stdout,"\n");
        va_end(ap);

    }
}

void registerBroadcasterTreeService(const char* pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::BroadcasterTree,"BroadcasterTree",getEvents_broadcasterTreeService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::BroadcasterTree,BroadcasterTree::Service::construct,"BroadcasterTree");
        regEvents_broadcasterTreeService();
    }
    XPASS;
}




bool BroadcasterTree::Service::SendToChild(const bcEvent::SendToChild*e, bool fromNetwork)
{
    sendEvent(e->dst_service,new bcEvent::Msg(e->payload,e->route));
    passEvent(new bcEvent::SendToChildAck(e->hash(),poppedFrontRoute(e->route)));
    make_broadcast_message_to_tree(e->dst_service,e->payload,e->bt,e->route);
    return true;
}
bool BroadcasterTree::Service::SendToChildAck(const bcEvent::SendToChildAck*e, bool fromNetwork)
{
    sendEvent(ServiceEnum::Timer,new timerEvent::StopAlarm(TIMER_BROADCAST_ACK_TIMEDOUT,toRef(e->hash),this));
    return true;
}
