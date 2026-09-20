#include "nodeService.h"
#include "tools_mt.h"
// #ifdef KALL
void Node::Service::broadcast_MsgEvent(const REF_getter<MsgData::Base>& b)
{
    std::string msg;
    // logErr2("b get %p",b.get());
    if(b.valid())
        msg=b->getBuffer();
    // logErr2("KALL 1");
    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    // auto nodes=buildTree()
    auto nn=db_state->getAllNodes();
    std::map<NODE_id,NodeElement> m;
    for(auto &z: nn)
    {
        m[z->getName()]=z->getElement();
    }
    auto tree=buildTree(m,this_node_name);
    sendEvent(
        ServiceEnum::BroadcasterTree,
        // ServiceEnum::Node,
        
              new bcEvent::BroadcastMessage(ServiceEnum::Node,
                                            this_node_name, node_start_timestamp, tree, seqId2++, signature,msg, ListenerBase::serviceId));

}
// #endif