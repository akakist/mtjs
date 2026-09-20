#include "nodeService.h"
#include "tools_mt.h"
// #ifdef KALL
void Node::Service::broadcast_MsgEvent(const REF_getter<MsgData::Base>& b, const std::set<NODE_id>& nodes)
{
    std::string msg;
    // logErr2("b get %p",b.get());
    if(b.valid())
        msg=b->getBuffer();
    // logErr2("KALL 1");
    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    sendEvent(
        ServiceEnum::BroadcasterTree,
        // ServiceEnum::Node,
        
              new bcEvent::BroadcastMessage(ServiceEnum::Node,
                                            this_node_name, node_start_timestamp, nodes, seqId2++, signature,msg, ListenerBase::serviceId));

}
// #endif