#include "base62.h"
#include "commonError.h"
#include "NODE_id.h"
#include "REF.h"
#include "ioBuffer.h"
#include "msg.h"
#include "bcEvent.h"
#include "listenerBase.h"
#include "mutexInspector.h"
#include "nodeService.h"
#include <SQLiteCpp/Statement.h>
#include <vector>
#include <cstdlib>
#include <cstddef>
#include <SQLiteCpp/Database.h>
#include "t_params.h"
#include "tools_mt.h"
#include "init_root.h"

void Node::Service::do_sync(const NODE_id &src_node)
{
    // if (!last_leader_cert.valid())
    //     throw CommonError("if(last_leader_cert.empty())");
    // std::vector<NODE_id> node_names;
    // for (auto &z : last_leader_cert->nodes)
    // {
    //     if (z != this_node_name)
    //         node_names.push_back(z);
    // }
    // auto nn = node_names[rand() % last_leader_cert->nodes.size()];
    // logNode("selected node %s", nn.container.c_str());
    auto n = root->getNode(src_node, NULL);
    if (!n.valid())
        throw CommonError("if(!n.valid())");
    REF_getter<MsgEvt::GetSavedBlocksREQ> gbr = new MsgEvt::GetSavedBlocksREQ();
    gbr->epoch = root->getEpoch(NULL)->epoch;
    logNode("@@@@@@@@@@@@@@@@ REQUEST for blocks since %s", gbr->epoch.toString().c_str());
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
        logNode("Query FROM epoch %s", r->epoch.toString().c_str());
        SQLite::Statement query1(dbs, "SELECT epoch, data FROM blocks WHERE epoch>=" + r->epoch.toString() + " LIMIT 20");
        // query1.bind(1, r->epoch.toString());
        bool found = false;
        while (query1.executeStep())
        {
            found = true;
            // BigInt ep;
            auto epoch_ = query1.getColumn(0).getString();
            BigInt epoch;
            epoch.from_string(epoch_);
            logNode("loaded from DB block epoch %s", epoch_.c_str());
            std::string data = base62::decode(query1.getColumn(1).getString());
            inBuffer in(data);
            REF_getter<MsgEvt::BlockDBStore> bds = new MsgEvt::BlockDBStore();
            bds->unpack2(in);
            ret->blocks_Z.push_back({epoch, bds});
        }
        if (!found)
            throw CommonError("!----------------FOUND");
    }

    // SQLite::Statement query2(dbs, "SELECT epoch, data FROM blocks WHERE epoch>? order by epoch limit 100");
    // query2.bind(1, epoch.toString());

    // while (query2.executeStep())
    // {
    //     BigInt epoch = query2.getColumn(0).getInt();
    //     std::string data = base62::decode(query2.getColumn(1).getString());
    //     inBuffer in(data);
    //     REF_getter<MsgEvt::BlockDBStore> bds = new MsgEvt::BlockDBStore();
    //     bds->unpack2(in);
    //     ret->blocks_Z.push_back({epoch, bds});

    // }
    logNode("QUERY RESULTS %d", ret->blocks_Z.size());

    ret->lastEpoch = root->getEpoch(NULL)->epoch;
    msg::node_message_ed nm(ret->getBuffer(), this_node_name, my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(), poppedFrontRoute(route)));

    return true;
}

bool Node::Service::GetSavedBlocksRSP(const MsgEvt::GetSavedBlocksRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    logNode("prev_block_hash %s", prev_block_hash_Z.str().c_str());
    for (auto &z : r->blocks_Z)
    {
        // if(z.second->epoch!=z.first)
        //     throw CommonError("if(hb.epoch!=z.first)");
        logNode("recv block epoch %s", z.second->block_accepted_req->leader_certificateZ->heart_beat->epoch.toString().c_str());
        logNode("cur epoch %s", root->getEpoch(NULL)->epoch.toString().c_str());
        if (z.second->block_accepted_req->leader_certificateZ->heart_beat->prev_block_hash != prev_block_hash_Z)
        {

            logNode("inval root hash %s %s", z.second->block_accepted_req->leader_certificateZ->heart_beat->prev_block_hash.str().c_str(), prev_block_hash_Z.str().c_str());
            logNode("received invalid block %s", z.second->epoch.toString().c_str());
            continue;
        }
        else
            logNode("ok received block %s", z.second->epoch.toString().c_str());
        // throw CommonError("if(hb.epoch!=root->getValues(NULL)->epoch) %s %s",z.second->epoch.toString().c_str(), root->getEpoch(NULL)->epoch.toString().c_str()   );

        std::vector<blst_cpp::PublicKey> agg_pk;
        for (auto &k : z.second->block_accepted_req->node_validators)
        {
            auto n = root->getNode(k, NULL);
            agg_pk.push_back(n->bls_pk);
        }
        if (!z.second->block_accepted_req->agg_sig.verify(agg_pk, blake2b_hash(z.second->block_accepted_req->block_payload->getBuffer()).container))
        {
            throw CommonError("on_get_blocks_rsp: !ba.agg_sig.verify");
        }
        // logNode("on_get_blocks_rsp: block verified OK");
        t_params t(root);
        execute_block(t, root, prev_block_hash_Z, z.second->att_data.trs, z.second->block_accepted_req->leader_certificateZ->nodes);
        blockDBStore = prepareBlockDBStore(z.second->att_data.trs, t, z.second->block_accepted_req->leader_certificateZ->nodes);
        // auto ep=root->getEpoch(NULL);
        // ep->epoch+=1;
        // ep->setDirty();
        auto new_root_hash = proceed_merkle_on_transaction_pool_hashers(root);

        if (new_root_hash == z.second->block_accepted_req->block_payload->new_root_hash1)
        {
            logNode("on_get_blocks_rsp: block executed OK on epoch %s", z.second->epoch.toString().c_str());
            // auto epoch = root->getEpoch(NULL);
            // auto new_epoch=epoch->copy();
            // epoch->epoch = z.second->epoch + 1;
            // epoch->setDirty();
            // root->setEpoch(new_epoch);
            // db->write_batch(db_to_save_Z);
            // db_to_save_Z.clear();
            // proceed_merkle_on_transaction_pool_hashers(root);

            logNode("db->write_batch(db_to_save_Z); %d", db_to_save_Z.cells.size());
            db->write_batch(db_to_save_Z);
            db_to_save_Z.clear();
            // this->Set
            // logNode("prev_block_hash_Z = new_root_hash;");
            prev_block_hash_Z = new_root_hash;
            // auto e=root->getEpoch(NULL)->epoch;
            // root->getEpoch(NULL)->epoch=e+1;
            // root->getEpoch(NULL)->setDirty();
            // new_root_hash = proceed_merkle_on_transaction_pool_hashers(root);
            // prev_block_hash_Z=new_root_hash;
            // db->write_batch(db_to_save_Z);
            // db_to_save_Z.clear();
        }
        else
        {
            root = new root_data(db.get());
            init_root(root);
            do_sync(src_node);
            logNode("if(new_root_hash!=bl.new_root_hash1) %s %s", new_root_hash.str().c_str(), z.second->block_accepted_req->block_payload->new_root_hash1.str().c_str());
            return true;
        }

        SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        logNode("insert epoch %s", z.second->epoch.toString().c_str());
        // SQLite::Statement insert(dbs, "REPLACE INTO blocks (epoch, data) VALUES (?, ?)");
        // insert.bind(1, z.second->epoch.toString());
        // insert.bind(2, z.second->getBuffer());
        // insert.exec();
        ///////////
        SQLite::Statement insert(dbs, "REPLACE INTO blocks (epoch, prev_root_hash, date, data) VALUES (?, ?, ?, ?)");
        insert.bind(1, z.second->epoch.toString());
        insert.bind(2, base62::encode(z.second->block_accepted_req->leader_certificateZ->heart_beat->prev_block_hash.container));
        insert.bind(3, time(NULL));
        insert.bind(4, base62::encode(z.second->getBuffer()));
        insert.exec();
    }
    if (r->lastEpoch > root->getEpoch(NULL)->epoch)
    {
        logNode("call3 do_sync();");

        logNode("do_sync again: r.lastEpoch %s > root->getValues(NULL)->epoch %s", r->lastEpoch.toString().c_str(), root->getEpoch(NULL)->epoch.toString().c_str());
        do_sync(src_node);
        return true;
    }
    state_Z = State::NORMAL;
    logNode("State::NORMAL");
    XPASS;
    return true;
}
