#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "commonError.h"
#include <exception>
#include "REF.h"
#include "IUtils.h"
#include "SERVICE_id.h"
#include "IInstance.h"
#include "broadcaster.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include "unknown.h"
#include <exception>
#include <cstdarg>
#include <cstdio>
#include <time.h>
#include <map>
#include "broadcasterTreeService.h"
#include "event_mt.h"
#include "listenerBuffered1Thread.h"
#include "route_t.h"
#include "tools_mt.h"
#include "tree.h"
#include "events_broadcasterTreeService.hpp"
#include "unknown.h"
#include "init_root.h"
bool BroadcasterTree::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;

    return true;
}

bool BroadcasterTree::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool BroadcasterTree::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;
    switch (e->tid)
    {
    case TIMER_BROADCAST_ACK_TIMEDOUT:
    {
        MUTEX_INSPECTOR;
        bcEvent::SendToChild *c = dynamic_cast<bcEvent::SendToChild *>(e->cookie.get());
        if (!c)
            throw CommonError("if(!c) 1222447");

        logErr2("TIMER_BROADCAST_ACK_TIMEDOUT %s", c->dstNodeName.container.c_str());

        make_broadcast_message_to_tree(c->dst_service,c->node_signer,c->payload_signature,  c->payload, c->bt, c->route);

        return true;
    }
    break;
    }
    return false;

    return false;
}

bool BroadcasterTree::Service::handleEvent(const REF_getter<Event::Base> &e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try
    {
        MUTEX_INSPECTOR;
        auto &ID = e->id;
        switch (ID)
        {
        case bcEventEnum::NodeMsgRSP:
            return NodeMsgRSP((const bcEvent::NodeMsgRSP *)e.get());
        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot *)e.get());
        case bcEventEnum::BroadcastMessage:
            return BroadcastMessage((const bcEvent::BroadcastMessage *)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit *)e.get());
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer *)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm *)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService *)e.get());
        case bcEventEnum::SendToChild:
            return SendToChild(static_cast<const bcEvent::SendToChild *>(e.get()), false);
        case bcEventEnum::SendToChildAck:
            return SendToChildAck(static_cast<const bcEvent::SendToChildAck *>(e.get()), false);

        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor *ev = static_cast<const rpcEvent::IncomingOnAcceptor *>(e.get());
            auto &IDA = ev->e->id;

            switch (IDA)
            {
            case bcEventEnum::NodeMsgRSP:
                return NodeMsgRSP((const bcEvent::NodeMsgRSP *)ev->e.get());
            case bcEventEnum::SendToChild:
                return SendToChild(static_cast<const bcEvent::SendToChild *>(ev->e.get()), true);
            case bcEventEnum::SendToChildAck:
                return SendToChildAck(static_cast<const bcEvent::SendToChildAck *>(ev->e.get()), true);

            default:
                throw CommonError("unhabdled ev %d %s", IDA, iUtils->genum_name(IDA));
            }
        }
        break;
        case rpcEventEnum::IncomingOnConnector:
        {
            const rpcEvent::IncomingOnConnector *ev = static_cast<const rpcEvent::IncomingOnConnector *>(e.get());
            auto &IDC = ev->e->id;
            switch (IDC)
            {
            case bcEventEnum::NodeMsgRSP:
                return NodeMsgRSP((const bcEvent::NodeMsgRSP *)ev->e.get());
            case bcEventEnum::SendToChild:
                return SendToChild(static_cast<const bcEvent::SendToChild *>(ev->e.get()), true);
            case bcEventEnum::SendToChildAck:
                return SendToChildAck(static_cast<const bcEvent::SendToChildAck *>(ev->e.get()), true);

            default:
                throw CommonError("unhabdled ev %d %s", IDC, iUtils->genum_name(IDC));
            }
        }
        break;

        default:
            throw CommonError("unhabdled ev %d %s", ID, iUtils->genum_name(ID));
        }
    }
    catch (std::exception &e)
    {
        logErr2("BroadcasterTree std::exception  %s", e.what());
    }
    XPASS;
    return false;
}

BroadcasterTree::Service::~Service()
{
}

BroadcasterTree::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins)
{
}

bool BroadcasterTree::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf = e;
    // if (!root.valid())
    //     root = getRoot(conf->db.get());

    // init_root(root);
    root=e->root;
    return true;
}
bool BroadcasterTree::Service::InvalidateRoot(const bcEvent::InvalidateRoot *e)
{
    root=e->root;
    // root = getRoot(conf->db.get());
    // init_root(root);
    return true;
}
bool BroadcasterTree::Service::BroadcastMessage(const bcEvent::BroadcastMessage *e)
{
    MUTEX_INSPECTOR;
    std::map<NODE_id, NodeElement> nodes;
// logErr2("BroadcastMessage from %s", e->node_signer.container.c_str());
    auto ks = root->getAllNodes();
    for (auto &nd : ks)
    {
        // auto nn = root->getNode(nd);
        if(nd->get_missed_rounds()>=100)
        {
            /// TODO
            // continue;

        }
        NodeElement n=nd->getElement();
        // n.name = nd->getName();
        // n.stake_A = nd->get_full_stake();
        // n.ip = nd->get_ip();
        // n.missed_rounds = nd->get_missed_rounds();
        nodes[nd->getName()] = n;
    }
    if (nodes.size() == 0)
        return true;
    BroadcasterTree::TreeNode root = BroadcasterTree::buildTree(nodes, conf->this_node_name);
    // logErr2("BroadcastMessage tree built with root %s", root.node.name.container.c_str());
    make_broadcast_message_to_tree(e->dstService,e->node_signer,e->signature_pl, e->msg, root, e->route);
    return true;
}
void BroadcasterTree::Service::make_broadcast_message_to_tree(SERVICE_id dstService, const NODE_id & node_signer, const std::string& signature, const std::string &msg, const BroadcasterTree::TreeNode &root, const route_t &route)
{
    MUTEX_INSPECTOR;
    auto &ch = root.children;
    for (auto it = ch.begin(); it != ch.end(); it++)
    {
        MUTEX_INSPECTOR;
        REF_getter<bcEvent::SendToChild> e1 = new bcEvent::SendToChild(node_signer,signature, msg, *it, dstService, it->node.name, route);
        REF_getter<bcEvent::SendToChild> e2 = new bcEvent::SendToChild(node_signer,signature, msg, *it, dstService, it->node.name, route);
        sendEvent(it->node.ip, ServiceEnum::BroadcasterTree, e1.get());

        sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(TIMER_BROADCAST_ACK_TIMEDOUT,
                  toRef(e2->hash()), e2.get(), BROADCAST_ACK_TIMEDOUT_SEC, this));
    }
}

bool BroadcasterTree::Service::NodeMsgRSP(const bcEvent::NodeMsgRSP *e)
{
    if (e->route.size())
    {
        passEvent(e);
        return true;
    }
    else
        throw CommonError("if(e->route.size())");
    return true;
}
void BroadcasterTree::Service::logNode(const char *fmt, ...)
{

    {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stdout, "%ld [BroadcasterTree] [%s] ", time(NULL),
                conf->this_node_name.container.c_str());
        vfprintf(stdout, fmt, ap);
        fprintf(stdout, "\n");
        va_end(ap);
    }
}

void registerBroadcasterTreeService(const char *pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if (pn)
    {
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::BroadcasterTree, "BroadcasterTree", getEvents_broadcasterTreeService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::BroadcasterTree, BroadcasterTree::Service::construct, "BroadcasterTree");
        regEvents_broadcasterTreeService();
    }
    XPASS;
}

bool BroadcasterTree::Service::SendToChild(const bcEvent::SendToChild *e, bool fromNetwork)
{
    // logNode("SendToChild from %s to %s", e->node_signer.container.c_str(), e->dstNodeName.container.c_str());
    sendEvent(e->dst_service, new bcEvent::NodeMsgREQ(e->node_signer,e->payload_signature, e->payload, e->route));
    passEvent(new bcEvent::SendToChildAck(e->hash(), poppedFrontRoute(e->route)));
    make_broadcast_message_to_tree(e->dst_service, e->node_signer,e->payload_signature, e->payload, e->bt, e->route);
    return true;
}
bool BroadcasterTree::Service::SendToChildAck(const bcEvent::SendToChildAck *e, bool fromNetwork)
{
    sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(TIMER_BROADCAST_ACK_TIMEDOUT, toRef(e->hash), this));
    return true;
}
