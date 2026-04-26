#ifdef KALL
#include "nodeService.h"
void Node::Service::make_broadcast_message_to_tree(const std::string & msg, const BroadcasterTree::TreeNode& root, const route_t& route)
{
    MUTEX_INSPECTOR;
    auto &ch=root.children;
    // logErr2("ch.size %d",ch.size());
    for(auto it=ch.begin(); it!=ch.end(); it++)
    {
        MUTEX_INSPECTOR;
        // logErr2("send to %s",it->node.name.c_str());
        msg::broadcast_tree bt;
        bt.tree=*it;
        bt.payload=msg;

        msg::node_message_ed nm(bt.getBuffer(),this_node_name,my_sk_ed);
        auto sb=nm.getBuffer();
        sendEvent(it->node.ip, ServiceEnum::Node, new bcEvent::Msg(sb,route));


        REF_getter<TIMER_BROADCAST_ACK_TIMEDOUT_cookie> cc=new TIMER_BROADCAST_ACK_TIMEDOUT_cookie;
        cc->dstName_=it->node.name;
        cc->msg=msg;
        cc->tree=(*it);
        cc->route=route;
        sendEvent(ServiceEnum::Timer,new timerEvent::SetAlarm(TIMER_BROADCAST_ACK_TIMEDOUT,
                  toRef(blake2b_hash(sb).container), cc.get(), BROADCAST_ACK_TIMEDOUT_SEC, this));

    }

}

void Node::Service::make_broadcast_message(const std::string & msg)
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
        return;
    BroadcasterTree::TreeNode root = BroadcasterTree::buildTree(nodes, this_node_name);

    make_broadcast_message_to_tree(msg,root, ListenerBase::serviceId);
}

void Node::Service::make_broadcast_message(const std::vector<uint8_t> & msg)
{
    MUTEX_INSPECTOR;
    make_broadcast_message({(char*)msg.data(),msg.size()});
}
#endif