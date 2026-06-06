#include "base16.h"
#include "blake2bHasher.h"
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
    auto n = root->getNode(src_node);
    if (!n.valid())
        throw CommonError("if(!n.valid())");
    REF_getter<MsgData::GetSavedBlocksREQ> gbr = new MsgData::GetSavedBlocksREQ();
    gbr->epoch = root->getEpoch()->epoch;
    gbr->prev_block_hash=prev_block_hash_Z;
    logNode("@@@@@@@@@@@@@@@@ REQUEST for blocks since %s", gbr->epoch.toString().c_str());
    // msg::node_message_ed nm(gbr->getBuffer(), this_node_name, my_sk_ed);
    // broadcast_MsgEvent(gbr);
    // send
    auto buffer = gbr->getBuffer();

    sendEvent(n->ip, ServiceEnum::Node,
              new bcEvent::NodeMsgREQ(this_node_name, sign_ed(my_sk_ed, blake2b_hash(buffer).container), buffer, ListenerBase::serviceId));
}

bool Node::Service::GetSavedBlocksREQ(const MsgData::GetSavedBlocksREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READONLY);

    REF_getter<MsgData::GetSavedBlocksRSP> ret = new MsgData::GetSavedBlocksRSP();

    BigInt epoch = 0;
    {
        logNode("Query FROM epoch %s", r->epoch.toString().c_str());
        SQLite::Statement query0(dbs, "SELECT epoch FROM blocks WHERE prev_root_hash='" + base16::encode(r->prev_block_hash.container) + "'");

        bool f1=false;
        BigInt epoch;
        while(query0.executeStep())
        {
            f1=true;
            epoch.from_string(query0.getColumn(0).getString());
        }
        if(!f1)
        {
            throw CommonError("NOT FOUND %s",base16::encode(r->prev_block_hash.container).c_str());
        }
        SQLite::Statement query1(dbs, "SELECT epoch,data FROM blocks WHERE epoch>=? ORDER by epoch");
        query1.bind(1, epoch.toString());
        bool found = false;
        while (query1.executeStep())
        {
            found = true;
            // BigInt ep;
            auto epoch_ = query1.getColumn(0).getString();
            BigInt epoch;
            epoch.from_string(epoch_);
            logNode("loaded from DB block epoch %s", epoch_.c_str());
            std::string data = base16::decode(query1.getColumn(1).getString());
            inBuffer in(data);
            REF_getter<MsgData::BlockDBStore> bds = new MsgData::BlockDBStore();
            bds->unpack2(in);
            ret->blocks_Z.push_back({epoch, bds});
        }
        if (!found)
            throw CommonError("!----------------FOUND");
    }

    logNode("QUERY RESULTS %d", ret->blocks_Z.size());

    ret->lastEpoch = root->getEpoch()->epoch;
    pass_NodeMsgRSP(ret.get(),route);

    return true;
}

bool Node::Service::GetSavedBlocksRSP(const MsgData::GetSavedBlocksRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    logNode("prev_block_hash %s", prev_block_hash_Z.str().c_str());
    for (auto &z : r->blocks_Z)
    {
        // if(z.second->epoch!=z.first)
        //     throw CommonError("if(hb.epoch!=z.first)");
        logNode("recv block epoch %s", z.second->validateBlockREQ->leader_cert->heart_beat->new_epoch.toString().c_str());
        logNode("cur epoch %s", root->getEpoch()->epoch.toString().c_str());

        logNode("recv prev_root_hash %s",z.second->validateBlockREQ->leader_cert->heart_beat->prev_block_hash.str().c_str());
        if(prev_block_hash_Z!=z.second->validateBlockREQ->leader_cert->heart_beat->prev_block_hash)
        {
            logNode("prev root hash not matched");
            continue;
        }
        else
            logNode("prev root hash matched !!!");
        if (z.second->validateBlockREQ->leader_cert->heart_beat->prev_block_hash != prev_block_hash_Z)
        {

            logNode("inval root hash %s %s", z.second->validateBlockREQ->leader_cert->heart_beat->prev_block_hash.str().c_str(), prev_block_hash_Z.str().c_str());
            // logNode("received invalid block %s", z.second->epoch.toString().c_str());
            continue;
        }
        else
            logNode("ok received block %s", z.second->blockAcceptedREQ->blockInfo->prev_epoch.toString().c_str());
        // throw CommonError("if(hb.epoch!=root->getValues(NULL)->epoch) %s %s",z.second->epoch.toString().c_str(), root->getEpoch(NULL)->epoch.toString().c_str()   );

        std::vector<blst_cpp::PublicKey> agg_pk;
        for (auto &k : z.second->blockAcceptedREQ->node_validators)
        {
            auto n = root->getNode(k);
            agg_pk.push_back(n->bls_pk);
        }
        if (!z.second->blockAcceptedREQ->agg_sig.verify(agg_pk, blake2b_hash(z.second->blockAcceptedREQ->blockInfo->getBuffer()).container))
        {
            throw CommonError("on_get_blocks_rsp: !ba.agg_sig.verify");
        }
        // logNode("on_get_blocks_rsp: block verified OK");
        t_params t(root);
        // t.att_data->trs = z.second->att_data->trs;
        t.validateBlockREQ = z.second->validateBlockREQ;
        auto rh=execute_block(t,  z.second->validateBlockREQ->leader_cert->nodes);
        // blockDBStore = prepareBlockDBStore(t);
        /*
        execute_block(t, root, prev_block_hash_Z, r->leader_cert->nodes);

        auto newEpoch = root->getEpoch();
        newEpoch->epoch += 1;
        newEpoch->setDirty(NULL);

        blockDBStore = prepareBlockDBStore(t);

        auto new_root_hash = proceed_merkle_on_transaction_pool_hashers(root);

        */
        auto new_root_hash = proceed_merkle_on_transaction_pool_hashers(root);

        if (new_root_hash == z.second->blockAcceptedREQ->blockInfo->new_root_hash1)
        {
            logNode("on_get_blocks_rsp: block executed OK on epoch %s", z.second->validateBlockREQ->leader_cert->heart_beat->new_epoch.toString().c_str());

            logNode("db->write_batch(db_to_save_Z); %d", db_to_save_Z.cells.size());
            db->write_batch(db_to_save_Z);
            db_to_save_Z.clear();
            prev_block_hash_Z = new_root_hash;
        }
        else
        {
            root = new root_data(db.get());
            init_root(root);
            do_sync(src_node);
            logNode("if(new_root_hash!=bl.new_root_hash1) %s %s", new_root_hash.str().c_str(), z.second->blockAcceptedREQ->blockInfo->new_root_hash1.str().c_str());
            return true;
        }

        SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        logNode("insert epoch %s", z.second->blockAcceptedREQ->blockInfo->prev_epoch.toString().c_str());
        ///////////
        SQLite::Statement insert(dbs, "REPLACE INTO blocks (epoch, prev_root_hash, date, data) VALUES (?, ?, ?, ?)");
        insert.bind(1, z.second->validateBlockREQ->leader_cert->heart_beat->new_epoch.toString());
        insert.bind(2, base16::encode(z.second->validateBlockREQ->leader_cert->heart_beat->prev_block_hash.container));
        insert.bind(3, time(NULL));
        insert.bind(4, base16::encode(z.second->getBuffer()));
        insert.exec();
    }
    if (r->lastEpoch > root->getEpoch()->epoch)
    {
        logNode("call3 do_sync();");

        logNode("do_sync again: r.lastEpoch %s > root->getValues(NULL)->epoch %s", r->lastEpoch.toString().c_str(), root->getEpoch()->epoch.toString().c_str());
        do_sync(src_node);
        return true;
    }
    else
        logNode("ERR %s %d", __FILE__, __LINE__);

    state_Z = State::NORMAL;
    logNode("State::NORMAL");
    XPASS;
    return true;
}
