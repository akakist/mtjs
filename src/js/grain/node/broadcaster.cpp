#include "nodeService.h"
#include "tools_mt.h"

/// Единственный живой путь броадкаста: нода отдаёт событие сервису BroadcasterTree,
/// он строит дерево и рассылает SendToChild, а ноды получают полезную нагрузку
/// как NodeMsgREQ. Старая реализация дерева прямо в ноде (make_broadcast_message_to_tree,
/// SendToChild/SendToChildAck, handle_send_to_child) удалена как рудимент.
void Node::Service::broadcast_MsgEvent_via_broadcaster(const REF_getter<MsgData::Base>& b,  const TreeNode &tree)
{
    MUTEX_INSPECTOR;

    std::string msg;
    if(b.valid())
        msg=b->getBuffer();
    auto signature=sign_ed(my_sk_ed,blake2b_hash(msg).container);
    sendEvent(
        ServiceEnum::BroadcasterTree,
        new bcEvent::BroadcastMessage(ServiceEnum::Node,
                                      this_node_name, node_start_timestamp, tree, seqId2++, signature,msg, ListenerBase::serviceId));

}
