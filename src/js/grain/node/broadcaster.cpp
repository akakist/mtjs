#include "nodeService.h"
#include "tools_mt.h"

void Node::Service::broadcast_MsgEvent_via_broadcaster(const REF_getter<MsgData::Base>& b, const REF_getter<Node::BlockMetaFull>& meta)
{
    std::string msg;
    if(b.valid())
        msg=b->getBuffer();
    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    // auto meta=getMetaFull();
    sendEvent(
        ServiceEnum::BroadcasterTree,
              new bcEvent::BroadcastMessage(ServiceEnum::Node,
                                            this_node_name, node_start_timestamp, meta->tree, seqId2++, signature,msg, ListenerBase::serviceId));

}
void Node::Service::broadcast_MsgEvent_via_node(const REF_getter<MsgData::Base>& b, const REF_getter<Node::BlockMetaFull>& meta)
{
    std::string msg;
    if(b.valid())
        msg=b->getBuffer();
    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    // auto meta=getMetaFull();
        make_broadcast_message_to_tree(ServiceEnum::Node,this_node_name, node_start_timestamp,seqId2++,signature, msg, meta->tree, ListenerBase::serviceId);
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
    passEvent(new bcEvent::SendToChildAck(e->hash(), poppedFrontRoute(e->route)));
    bool need_continue_broadcast=true;
    handle_send_to_child(e->node_signer,e->node_start_timestamp,e->seqId2,e->payload_signature,e->payload,e->route, & need_continue_broadcast);
    if(need_continue_broadcast)
        make_broadcast_message_to_tree(e->dst_service, e->node_signer, e->node_start_timestamp,e->seqId2,e->payload_signature, e->payload, e->bt, e->route);
    return true;
}
bool Node::Service::SendToChildAck(const bcEvent::SendToChildAck *e, bool fromNetwork)
{
    sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(TIMER_BROADCAST_ACK_TIMEDOUT, toRef(e->hash), this));
    return true;
}

bool Node::Service::handle_send_to_child(const NODE_id &node_signer, int64_t node_start_timestamp, int64_t seqId, const std::string &signature,
            const std::string &msg,
            const route_t &route, bool *need_continue_broadcast)
{
    auto &s=filter_NodeMsgREQ[node_signer][node_start_timestamp];
    while(s.size()>100)
    {
        s.erase(s.begin());
    }
    if(s.count(seqId))
    {
        logNode("filter_NodeMsgREQ.count(seqId) %lld",seqId);
        return true;
    }
    s.insert(seqId);
    auto n = db_state->getNodeNoCreate(node_signer);
    if(!n.valid())
        return true;
    if (!verify_ed_pk(n->get_ed_pk(), signature, blake2b_hash(msg)))
    {
        logNode("verify failed 1123");
        return true;
    }
    inBuffer in(msg);
    auto id1 = in.get_PN();

    REF_getter<MsgData::Base> msgb = msgFactory.create(id1);
    msgb->unpack(in);

    switch (msgb->type)
    {
    case msgid::LcEnvelopeREQ:
    return LcEnvelopeREQ(static_cast<const MsgData::LcEnvelopeREQ *>(msgb.get()), node_signer, route,need_continue_broadcast);

    default:
        throw CommonError("unjandled323 MsgData %s", msgName(msgb->type));
    }

    return true;

}

