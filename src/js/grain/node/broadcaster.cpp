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
        ServiceEnum::BroadcasterTree,
        // ServiceEnum::Node,
        
              new bcEvent::BroadcastMessage(ServiceEnum::Node,
                                            this_node_name, node_start_timestamp, meta->tree, seqId2++, signature,msg, ListenerBase::serviceId));

}
// #endif