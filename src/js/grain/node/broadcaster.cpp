#include "nodeService.h"
#include "tools_mt.h"
// #ifdef KALL
void Node::Service::broadcast_MsgEvent(const REF_getter<MsgData::Base>& b)
{
    std::string msg;
    if(b.valid())
        msg=b->getBuffer();
    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    auto meta=getMetaFull();
    sendEvent(
        // ServiceEnum::BroadcasterTree,
        ServiceEnum::Node,
        
              new bcEvent::BroadcastMessage(ServiceEnum::Node,
                                            this_node_name, node_start_timestamp, meta->tree, seqId2++, signature,msg, ListenerBase::serviceId));

}
bool Node::Service::BroadcastMessage(const bcEvent::BroadcastMessage*e)
{
    make_broadcast_message_to_tree(e->dstService,e->node_signer, e->node_start_timestamp,e->seqId,e->signature_pl, e->msg, e->nodes, e->route);
    return true;
}
void Node::Service::make_broadcast_message_to_tree(SERVICE_id dstService, const NODE_id & node_signer, int64_t node_start_timestamp, int64_t seqId, const std::string& signature, const std::string &msg, const TreeNode &root, const route_t &route)
{
    MUTEX_INSPECTOR;
    auto &ch = root.children;
    for (auto it = ch.begin(); it != ch.end(); it++)
    {
        MUTEX_INSPECTOR;
        REF_getter<bcEvent::SendToChild> e1 = new bcEvent::SendToChild(node_signer, node_start_timestamp, seqId, signature, msg, *it, dstService, it->node.name, route);
        REF_getter<bcEvent::SendToChild> e2 = new bcEvent::SendToChild(node_signer, node_start_timestamp, seqId, signature, msg, *it, dstService, it->node.name, route);
        sendEvent(it->node.ip, ServiceEnum::Node, e1.get());

        sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(TIMER_BROADCAST_ACK_TIMEDOUT,
                  toRef(e2->hash()), e2.get(), BROADCAST_ACK_TIMEDOUT_SEC, this));
    }
}
bool Node::Service::SendToChild(const bcEvent::SendToChild *e, bool fromNetwork)
{
    // logNode("SendToChild from %s to %s", e->node_signer.container.c_str(), e->dstNodeName.container.c_str());
    sendEvent(e->dst_service, new bcEvent::NodeMsgREQ(e->node_signer, e->node_start_timestamp, e->seqId2, e->payload_signature, e->payload, e->route));
    passEvent(new bcEvent::SendToChildAck(e->hash(), poppedFrontRoute(e->route)));
    make_broadcast_message_to_tree(e->dst_service, e->node_signer, e->node_start_timestamp,e->seqId2,e->payload_signature, e->payload, e->bt, e->route);
    return true;
}
bool Node::Service::SendToChildAck(const bcEvent::SendToChildAck *e, bool fromNetwork)
{
    sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(TIMER_BROADCAST_ACK_TIMEDOUT, toRef(e->hash), this));
    return true;
}

// #endif