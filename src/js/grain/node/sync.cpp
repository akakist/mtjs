#include "base16.h"
#include "blake2bHasher.h"
#include "commonError.h"
#include "NODE_id.h"
#include "REF.h"
#include "ioBuffer.h"
#include "md_GetSavedBlocksRSP.h"
#include "msg.h"
#include "bcEvent.h"
#include "listenerBase.h"
#include "mutexInspector.h"
#include "nodeService.h"
#include <vector>
#include <cstdlib>
#include <cstddef>
#include "t_params.h"
#include "tools_mt.h"
#include "init_root.h"
#include "base16.h"

void Node::Service::do_sync(const NODE_id &src_node)
{
    if(state_Z==SYNCING)
        return;
    auto n = root->getNode(src_node);
    if (!n.valid())
        throw CommonError("if(!n.valid())");
    REF_getter<MsgData::GetSavedBlocksREQ> gbr = new MsgData::GetSavedBlocksREQ();
    // gbr->epoch = root->getEpoch()->epoch;
    gbr->prev_root_hash = prev_root_hash_Z;
    logNode("do_sync: @@@@@@@@@@@@@@@@ REQUEST for blocks since '%s'", gbr->prev_root_hash.str().c_str());
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
    if(state_Z==SYNCING)
        return true;

    // if(state_Z==SYNCING)
    //     return;
    // SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READONLY);

    REF_getter<MsgData::GetSavedBlocksRSP> ret = new MsgData::GetSavedBlocksRSP();

    BigInt epoch = 0;
    {
            MUTEX_INSPECTOR;
        logNode("Query FROM root hash '%s' blocks %d", r->prev_root_hash.str().c_str(),ret->blocks_ZZ.size());
        BLOCK_id prev = r->prev_root_hash;
        while (ret->blocks_ZZ.size() < 40)
        {

            std::string res;
            if (db_history->get_cell(prev.container, &res))
            {
                logNode("cannot find block %s", r->prev_root_hash.str().c_str());
                break;
            }

            REF_getter<MsgData::BlockDBStore> bs;
            {
                MUTEX_INSPECTOR;
                inBuffer in(res);
                in >> bs;

            }
            logNode("append to ret->blocks_ZZ");
            ret->blocks_ZZ.push_back(bs);
            prev = bs->blockAcceptedREQ->blockInfo->new_root_hash1;
        }
        ret->last_prev_root_hash=prev_root_hash_Z;

    }

    logNode("GetSavedBlocksREQ QUERY RESULTS size %d", ret->blocks_ZZ.size());

    // ret->lastEpoch = root->getEpoch()->epoch;
    pass_NodeMsgRSP(ret.get(), route);

    return true;
}

bool Node::Service::GetSavedBlocksRSP(const MsgData::GetSavedBlocksRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if(state_Z!=SYNCING)
    {
        logNode("error if(state_Z!=SYNCING)");
    }
    logNode("prev_root_hash %s", prev_root_hash_Z.str().c_str());
    for (auto &z : r->blocks_ZZ)
    {
        logNode("iter block epoch %s", z->validateBlockREQ->leader_cert->heart_beat->new_epoch.toString().c_str());
        logNode("cur epoch %s", root->getEpoch()->epoch.toString().c_str());

        logNode("iter prev_root_hash in validateBlockREQ %s", z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash.str().c_str());
        logNode("iter prev_root_hash in blockAcceptedREQ %s", z->blockAcceptedREQ->blockInfo->prev_root_hash.str().c_str());
        if (prev_root_hash_Z != z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash)
        {
            logNode("prev root hash not matched '%s' != '%s'", prev_root_hash_Z.str().c_str(),z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash.str().c_str());
            continue;
        }
        else
            logNode("prev root hash matched !!!");
        if (z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash != prev_root_hash_Z)
        {

            logNode("inval root hash %s %s", z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash.str().c_str(), prev_root_hash_Z.str().c_str());
            // logNode("received invalid block %s", z.second->epoch.toString().c_str());
            continue;
        }
        else
            logNode("ok received block %s", z->blockAcceptedREQ->blockInfo->prev_epoch.toString().c_str());
        // throw CommonError("if(hb.epoch!=root->getValues(NULL)->epoch) %s %s",z.second->epoch.toString().c_str(), root->getEpoch(NULL)->epoch.toString().c_str()   );

        std::vector<blst_cpp::PublicKey> agg_pk;
        for (auto &k : z->blockAcceptedREQ->node_validators)
        {
            auto n = root->getNode(k);
            agg_pk.push_back(n->bls_pk);
        }
        if (!z->blockAcceptedREQ->agg_sig.verify(agg_pk, blake2b_hash(z->blockAcceptedREQ->blockInfo->getBuffer()).container))
        {
            throw CommonError("on_get_blocks_rsp: !ba.agg_sig.verify");
        }
        // logNode("on_get_blocks_rsp: block verified OK");
        t_params t(root);
        // t.att_data->trs = z.second->att_data->trs;
        t.validateBlockREQ = z->validateBlockREQ;
        auto rh = execute_block(t, z->validateBlockREQ->leader_cert->nodes);

        auto new_root_hash = proceed_merkle_on_transaction_pool_hashers(root);

        if (new_root_hash == z->blockAcceptedREQ->blockInfo->new_root_hash1)
        {
            logNode("on_get_blocks_rsp: block executed OK on epoch %s", z->validateBlockREQ->leader_cert->heart_beat->new_epoch.toString().c_str());

            logNode("db->write_batch(db_to_save_Z); %d", db_to_save_Z.cells.size());
            db_state->write_batch(db_to_save_Z);
            db_to_save_Z.clear();
            prev_root_hash_Z = new_root_hash;
        }
        else
        {
            root = new root_data(db_state.get());
            init_root(root);
            do_sync(src_node);
            logNode("if(new_root_hash!=bl.new_root_hash1) %s %s", new_root_hash.str().c_str(), z->blockAcceptedREQ->blockInfo->new_root_hash1.str().c_str());
            return true;
        }
        outBuffer o;
        o<<z;
        if(db_history->put_cell(z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash.container,
            o.asString()->container
        ))
        {
            logErr2("error saving block");
        }
    }
    if (r->last_prev_root_hash != prev_root_hash_Z)
    {
        logNode("call3 do_sync();");

        logNode("do_sync again: ");
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
