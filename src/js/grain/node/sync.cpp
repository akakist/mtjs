#include "blake2bHasher.h"
#include "commonError.h"
#include "NODE_id.h"
#include "REF.h"
#include "ioBuffer.h"
#include "md_GetSavedBlocksRSP.h"
#include "bcEvent.h"
#include "listenerBase.h"
#include "mutexInspector.h"
#include "nodeService.h"
#include <cstdlib>
#include <cstddef>
#include "init_root.h"
#include "md_DoYouHaveBlockREQ.h"
#include "md_DoYouHaveBlockRSP.h"

void Node::Service::do_sync(const NODE_id &src_node)
{
    MUTEX_INSPECTOR;
    logNode("void Node::Service::do_sync(const NODE_id &src_node) ");
    auto n = root->getNode(src_node);
    if (!n.valid())
        throw CommonError("if(!n.valid())");
    REF_getter<MsgData::GetSavedBlocksREQ> gbr = new MsgData::GetSavedBlocksREQ();
    gbr->prev_root_hash = prev_root_hash_Z;
    logNode("do_sync: @@@@@@@@@@@@@@@@ REQUEST for blocks since '%s'", gbr->prev_root_hash.str().c_str());
    auto buffer = gbr->getBuffer();

    sendEvent(n->get_ip(), ServiceEnum::Node,
              new bcEvent::NodeMsgREQ(this_node_name, sign_ed(my_sk_ed, blake2b_hash(buffer).container), buffer, ListenerBase::serviceId));
}
bool Node::Service::GetSavedBlocksREQ(const MsgData::GetSavedBlocksREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    if(state_Z==STATE_SYNCING)
        return true;


    REF_getter<MsgData::GetSavedBlocksRSP> ret = new MsgData::GetSavedBlocksRSP();

    BigInt epoch = 0;
    {
        MUTEX_INSPECTOR;
        logNode("Query FROM root hash '%s' blocks %d", r->prev_root_hash.str().c_str(),ret->blocks_ZZ.size());
        BLOCK_id prev = r->prev_root_hash;
        while (ret->blocks_ZZ.size() < 40)
        {

            std::string res;
            if (db_history->get(prev.container, &res))
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

    pass_NodeMsgRSP(ret.get(), route);

    return true;
}

bool Node::Service::GetSavedBlocksRSP(const MsgData::GetSavedBlocksRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if(state_Z!=STATE_SYNCING)
    {
        logNode("error if(state_Z!=SYNCING)");
    }
    logNode("prev_root_hash %s", prev_root_hash_Z.str().c_str());
    logNode("GetSavedBlocksRSP received blocks %d", r->blocks_ZZ.size());
    if(r->blocks_ZZ.size()==0)
    {
        /// blocks not found
        auto &s=syncs[prev_root_hash_Z];
        if(!s.do_you_have_sent)
        {
            broadcast_MsgEvent(new MsgData::DoYouHaveBlockREQ(prev_root_hash_Z));
            s.do_you_have_sent=true;    
            sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(timers::TIMER_SYNC_TIMEDOUT,NULL,NULL,3,this));
        }
        return true;
    }
    for (auto &z : r->blocks_ZZ)
    {
        logNode("iter block epoch %ld", z->validateBlockREQ->leader_cert->heart_beat->new_epoch);
        logNode("cur epoch %ld", root->getEpoch()->epoch);

        logNode("iter prev_root_hash in validateBlockREQ %s", z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash_1.str().c_str());
        logNode("iter prev_root_hash in blockAcceptedREQ %s", z->blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1.str().c_str());
        if (prev_root_hash_Z != z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash_1)
        {
            logNode("prev root hash not matched '%s' != '%s'", prev_root_hash_Z.str().c_str(),z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash_1.str().c_str());
            continue;
        }
        else
            logNode("prev root hash matched !!!");
        if (z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash_1 != prev_root_hash_Z)
        {

            logNode("inval root hash %s %s", z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash_1.str().c_str(), prev_root_hash_Z.str().c_str());
            continue;
        }
        else
            logNode("ok received block %ld", z->blockAcceptedREQ->blockInfo->heart_beat->new_epoch);

        std::vector<blst_cpp::PublicKey> agg_pk;
        for (auto &k : z->blockAcceptedREQ->node_validators)
        {
            auto n = root->getNode(k);
            agg_pk.push_back(n->get_bls_pk());
        }
        if (!z->blockAcceptedREQ->agg_sig.verify(agg_pk, blake2b_hash(z->blockAcceptedREQ->blockInfo->getBuffer()).container))
        {
            throw CommonError("on_get_blocks_rsp: !ba.agg_sig.verify");
        }
        logNode("on_get_blocks_rsp: block verified OK");
        t_params t(root);
        t.validateBlockREQ = z->validateBlockREQ;
        auto rh = execute_block(t, z->validateBlockREQ->leader_cert->nodes);

        auto new_root_hash = proceed_merkle_on_transaction_pool_hashers(root);

        if (new_root_hash == z->blockAcceptedREQ->blockInfo->new_root_hash1)
        {
            logNode("on_get_blocks_rsp: block executed OK on epoch %ld", z->validateBlockREQ->leader_cert->heart_beat->new_epoch);

            db_state->write_batch(db_to_save_Z);
            // sendEvent(ServiceEnum::GrainWriter,
            //     new bcEvent::WriteGranules(db_to_save_Z,
            //         t.validateBlockREQ->leader_cert->heart_beat->new_epoch,
            //         db_state,this));

            logNode("db->write_batch(db_to_save_Z); done %d granules", db_to_save_Z.cells.size());
            db_to_save_Z.clear();
            prev_root_hash_Z = new_root_hash;
        }
        else
        {
            root = new root_data(db_state.get());
            init_root(root);
            do_InvalidateRoot();
            if(state_Z==STATE_SYNCING)
            {
                return true;
            }
            state_Z=STATE_SYNCING;
            do_sync(src_node);
            logNode("if(new_root_hash!=bl.new_root_hash1) %s %s", new_root_hash.str().c_str(), z->blockAcceptedREQ->blockInfo->new_root_hash1.str().c_str());
            return true;
        }
        outBuffer o;
        o<<z;
        if(db_history->writeBlock(z->validateBlockREQ->leader_cert->heart_beat->new_epoch, z->validateBlockREQ->leader_cert->heart_beat->prev_root_hash_1.container,
                                  o.asString()->container
                                 ))
        {
            logNode("error saving block");
        }
    }
    if (r->last_prev_root_hash != prev_root_hash_Z)
    {
        logNode("call3 do_sync();");

        logNode("do_sync again: ");
        state_Z=STATE_SYNCING;
        do_sync(src_node);
        return true;
    }
    else
    {
    }

    logNode("!!! SUCCESS CATCH UP ");
    state_Z = State::STATE_NORMAL;
    logNode("State::NORMAL");
    XPASS;
    return true;
}


bool Node::Service::DoYouHaveBlockREQ(const MsgData::DoYouHaveBlockREQ* m, const NODE_id & src_node, const route_t& route)
{
    logErr2("@@ %s",__PRETTY_FUNCTION__);
    std::string res;
    if (db_history->get(m->prev_root_hash.container, &res))
    {
        logNode("cannot find block %s", m->prev_root_hash.str().c_str());
        return true;
    }
    pass_NodeMsgRSP(new MsgData::DoYouHaveBlockRSP(true,m->prev_root_hash), route);
    return true;
}
bool Node::Service::DoYouHaveBlockRSP(const MsgData::DoYouHaveBlockRSP* m, const NODE_id & src_node, const route_t& route)
{
    logErr2("@@ %s",__PRETTY_FUNCTION__);
    auto& s=syncs[m->prev_root_hash];
    s.havers.insert(src_node);
    if(s.havers.size()==1)
    {
        do_sync(src_node);
    }
    return true;
}
