#include "nodeService.h"
#include "tools_mt.h"
// #ifdef KALL
        // void broadcast_MsgEvent_via_broadcaster(const REF_getter<MsgData::Base>& p);
        // void (const REF_getter<MsgData::Base>& p);

void Node::Service::broadcast_MsgEvent_via_broadcaster(const REF_getter<MsgData::Base>& b)
{
    std::string msg;
    if(b.valid())
        msg=b->getBuffer();
    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    auto meta=getMetaFull();
    sendEvent(
        ServiceEnum::BroadcasterTree,
        // ServiceEnum::Node,
        
              new bcEvent::BroadcastMessage(ServiceEnum::Node,
                                            this_node_name, node_start_timestamp, meta->tree, seqId2++, signature,msg, ListenerBase::serviceId));

}
void Node::Service::broadcast_MsgEvent_via_node(const REF_getter<MsgData::Base>& b)
{
    std::string msg;
    if(b.valid())
        msg=b->getBuffer();
    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    auto meta=getMetaFull();
    // sendEvent(
        // ServiceEnum::BroadcasterTree,
        // ServiceEnum::Node,
        
            //   new bcEvent::BroadcastMessage(ServiceEnum::Node,
            //                                 this_node_name, node_start_timestamp, meta->tree, seqId2++, signature,msg, ListenerBase::serviceId));
        make_broadcast_message_to_tree(ServiceEnum::Node,this_node_name, node_start_timestamp,seqId2++,signature, msg, meta->tree, ListenerBase::serviceId);
}
// bool Node::Service::BroadcastMessage(const bcEvent::BroadcastMessage*e)
// {
//     make_broadcast_message_to_tree(e->dstService,e->node_signer, e->node_start_timestamp,e->seqId,e->signature_pl, e->msg, e->nodes, e->route);
//     return true;
// }
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
    passEvent(new bcEvent::SendToChildAck(e->hash(), poppedFrontRoute(e->route)));
    bool need_continue_broadcast=true;
    handle_send_to_child(e->node_signer,e->node_start_timestamp,e->seqId2,e->payload_signature,e->payload,e->route, & need_continue_broadcast);
    // sendEvent(e->dst_service, new bcEvent::NodeMsgREQ(e->node_signer, e->node_start_timestamp, e->seqId2, e->payload_signature, e->payload, e->route));
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
    {
            MsgData::LcEnvelopeREQ *le=(MsgData::LcEnvelopeREQ *)msgb.get();
            inBuffer in(le->msg);
            auto id2 = in.get_PN();
            REF_getter<MsgData::Base> msg = msgFactory.create(id2);
            msg->unpack(in);
            
            REF_getter<MsgData::BlockAcceptedREQ> lc;
            if(le->prev_lc.size())
            {
                lc=new MsgData::BlockAcceptedREQ;
                inBuffer in2(le->prev_lc);
                lc->unpack2(in2);

            }
    
            switch (msg->type)
            {
            case msgid::HeartBeatREQ:
            
                // last_activity_time=iUtils->getNow();
                return HeartBeatREQ(static_cast<const MsgData::HeartBeatREQ *>(msg.get()),lc.valid()?lc.get():NULL, node_signer, route, need_continue_broadcast);

            default:
                throw CommonError("2 MsgData %s", msgName(msg->type));
            }

    }
        // return LcEnvelopeREQ(static_cast<const MsgData::LcEnvelopeREQ *>(msgb.get()), node_signer, route);
    // case msgid::GetTransactionREQ:
    //     return GetTransactionREQ(static_cast<const MsgData::GetTransactionREQ *>(msg.get()), m->node_signer, m->route);
    // case msgid::ValidateBlockREQ:
    //     last_activity_time=iUtils->getNow();
    //     return ValidateBlockREQ(static_cast<const MsgData::ValidateBlockREQ *>(msg.get()), m->node_signer, m->route);
    // case msgid::BlockAcceptedREQ:
    //     last_activity_time=iUtils->getNow();
    //     return BlockAcceptedREQ(static_cast<const MsgData::BlockAcceptedREQ *>(msg.get()), m->node_signer, m->route);
    // case msgid::ConfirmLeaderREQ:
    //     return ConfirmLeaderREQ(static_cast<const MsgData::ConfirmLeaderREQ *>(msg.get()), m->node_signer, m->route);
    // case msgid::DelayNotificationREQ:
    //     return DelayNotificationREQ(static_cast<const MsgData::DelayNotificationREQ *>(msg.get()), m->node_signer, m->route);

    default:
        throw CommonError("unjandled323 MsgData %s", msgName(msgb->type));
    }

    return true;

}

// #endif