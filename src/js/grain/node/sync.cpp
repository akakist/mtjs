#include "commonError.h"
#include "NODE_id.h"
#include "REF.h"
#include "ioBuffer.h"
#include "msg.h"
#include "bcEvent.h"
#include "listenerBase.h"
#include "mutexInspector.h"
#include "nodeService.h"
#include <vector>
#include <cstdlib>
#include <cstddef>
#include <SQLiteCpp/Database.h>
#include "tools_mt.h"

void Node::Service::do_sync(const NODE_id &src_node)
{
    if (!last_leader_cert.valid())
        throw CommonError("if(last_leader_cert.empty())");
    std::vector<NODE_id> node_names;
    for (auto &z : last_leader_cert->nodes)
    {
        if (z != this_node_name)
            node_names.push_back(z);
    }
    auto nn = node_names[rand() % last_leader_cert->nodes.size()];
    logNode("selected node %s", nn.container.c_str());
    auto n = root->getNode(nn, NULL);
    if (!n.valid())
        throw CommonError("if(!n.valid())");
    REF_getter<MsgEvt::GetSavedBlocksREQ> gbr = new MsgEvt::GetSavedBlocksREQ();
    gbr->epoch = root->getEpoch(NULL)->epoch;
    msg::node_message_ed nm(gbr->getBuffer(), this_node_name, my_sk_ed);
    sendEvent(n->ip, ServiceEnum::Node, new bcEvent::Msg(nm.getBuffer(), ListenerBase::serviceId));
}

bool Node::Service::GetSavedBlocksREQ(const MsgEvt::GetSavedBlocksREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READONLY);

    REF_getter<MsgEvt::GetSavedBlocksRSP> ret = new MsgEvt::GetSavedBlocksRSP();

    BigInt epoch = 0;
    {
        SQLite::Statement query1(dbs, "SELECT epoch, data FROM blocks WHERE epoch>=?");
        query1.bind(1, r->epoch.toString());
        bool found = false;
        while (query1.executeStep())
        {
            found = true;
            // BigInt ep;
            BigInt epoch = query1.getColumn(0).getInt();
            std::string data = base62::decode(query1.getColumn(1).getString());
            inBuffer in(data);
            REF_getter<MsgEvt::BlockDBStore> bds = new MsgEvt::BlockDBStore();
            bds->unpack2(in);
            ret->blocks_Z.push_back({epoch, bds});
        }
        if (!found)
            throw CommonError("!----------------FOUND");
    }

    SQLite::Statement query2(dbs, "SELECT epoch, data FROM blocks WHERE epoch>? order by epoch limit 100");
    query2.bind(1, epoch.toString());

    while (query2.executeStep())
    {
        BigInt epoch = query2.getColumn(0).getInt();
        std::string data = base62::decode(query2.getColumn(1).getString());
        inBuffer in(data);
        REF_getter<MsgEvt::BlockDBStore> bds = new MsgEvt::BlockDBStore();
        bds->unpack2(in);
        ret->blocks_Z.push_back({epoch, bds});
    }

    ret->lastEpoch = root->getEpoch(NULL)->epoch;
    msg::node_message_ed nm(ret->getBuffer(), this_node_name, my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(), poppedFrontRoute(route)));

    return true;
}
