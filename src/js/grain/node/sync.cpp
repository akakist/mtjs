#include "nodeService.h"
#include <SQLiteCpp/Database.h>
#include "tools_mt.h"
#include "blst_cp.h"
#include "base62.h"

void Node::Service::do_sync()
{
    // last_leader_cert
    if(!last_leader_cert.valid()) throw CommonError("if(last_leader_cert.empty())");
    // msg::leader_certificate lc(last_leader_cert);
    // lc.nodes
    std::vector<NODE_id> node_names;
    for(auto& z: last_leader_cert->nodes)
    {
        if(z!=this_node_name)
            node_names.push_back(z);
    }
    auto nn=node_names[rand()%last_leader_cert->nodes.size()];
    logNode("selected node %s",nn.container.c_str());
    auto n=root->getNode(nn,NULL);
    if(!n.valid())
        throw CommonError("if(!n.valid())");
    REF_getter<MsgEvent::GetSavedBlocksREQ>  gbr=new MsgEvent::GetSavedBlocksREQ();
    gbr->myEpoch=root->getValues(NULL)->epoch;
    msg::node_message_ed nm(gbr->getBuffer(),this_node_name,my_sk_ed );
    sendEvent(n->ip,ServiceEnum::Node, new bcEvent::Msg(nm.getBuffer(),ListenerBase::serviceId));

}
