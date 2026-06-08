#include "NODE_id.h"
#include "REF.h"
#include "blst_cp.h"
#include "blake2bHasher.h"
#include "IUtils.h"
#include "commonError.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
// #include <SQLiteCpp/Statement.h>
#include <string>
#include <time.h>
#include "md_BlockAcceptedRSP.h"
#include "md_BlockDBStore.h"
#include "md_ValidateBlockRSP.h"
#include "ioBuffer.h"
#include "nodeService.h"
#include "route_t.h"
#include "s_ed.h"
#include "t_params.h"
#include "tools_mt.h"
// #include <SQLiteCpp/Database.h>
#include <vector>

bool Node::Service::BlockAcceptedREQ(const MsgData::BlockAcceptedREQ *r, const NODE_id &src_node, const route_t &route)
{
    if(state_Z!=STATE_NORMAL)
        return true;
    MUTEX_INSPECTOR;
    XTRY;
    // blockDBStore->validateBlockREQ->leader_cert
    auto& c=c_blocks[r->blockInfo->prev_root_hash];

    if (!c.blockDBStore.valid())
    {
        logErr2("if (!c.blockDBStore.valid())");
        return true;

    }

    if (c.blockDBStore->validateBlockREQ->leader_cert->heart_beat->node_leader != src_node)
    {
        logErr2("if(blockDBStore->validateBlockREQ->leader_cert->heart_beat->node_leader!=src_node)");
        return true;
    }

    if (CheckState(c.blockDBStore->validateBlockREQ->leader_cert->heart_beat.get(), src_node))
        return true;
    if (state_Z != State::STATE_NORMAL)
        return true;

    resetTimer();
    if (! c.blockDBStore.valid())
        throw CommonError("if (!blockDBStore.valid())");
    c.blockDBStore->blockAcceptedREQ = r;
    std::vector<blst_cpp::PublicKey> agg_pk;
    for (auto &z : r->node_validators)
    {
        XTRY;
        agg_pk.push_back(root->getNode(z)->bls_pk);
        XPASS;
    }

    {
        MUTEX_INSPECTOR;
        XTRY;
        if (!r->agg_sig.verify(agg_pk, blake2b_hash(r->blockInfo->getBuffer()).container))
        {
            logNode("block aggsig not matched");
            return true;
        }
        else
        {
            // logNode("block verified OK");
        }
        XPASS;
    }
    if(this_node_name==c.blockDBStore->validateBlockREQ->leader_cert->heart_beat->node_leader)
    {
        size_t sz=0;
        for(auto& z:db_to_save_Z.cells)
        {
            sz+=z.second.size();
        }
        logNode("db_state->write_batch %d granules, total size %d",db_to_save_Z.cells.size(),sz);
    }
    db_state->write_batch(db_to_save_Z);
    db_to_save_Z.clear();

    {
        MUTEX_INSPECTOR;
        XTRY;
        outBuffer o;
        o<<c.blockDBStore;
        db_history->writeBlock(c.blockDBStore->validateBlockREQ->leader_cert->heart_beat->new_epoch, c.blockDBStore->validateBlockREQ->leader_cert->heart_beat->prev_root_hash.container,
                               o.asString()->container
                              );
        XPASS;
    }

    sendEvent(ServiceEnum::BlockStreamer, new bcEvent::StreamBlock(c.blockDBStore, c.att_data, this));

    prev_root_hash_Z = r->blockInfo->new_root_hash1;
    blocks_leader.clear();
    node_leader_for_client.container.clear();
    sendEvent(ServiceEnum::TxValidator, new bcEvent::InvalidateRoot(this));
    sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::InvalidateRoot(this));
    sendEvent(ServiceEnum::GrainReader, new bcEvent::InvalidateRoot(this));

    {
        MUTEX_INSPECTOR;
        XTRY;
        REF_getter<MsgData::BlockAcceptedRSP> br = new MsgData::BlockAcceptedRSP();
        br->new_root_hash = prev_root_hash_Z;
        br->node_signer = this_node_name;
        br->sign(my_sk_bls);

        resetTimer();
        pass_NodeMsgRSP(br.get(), route);
        XPASS;
    }

    iUtils->getNow();

    for (auto &z : c.blockDBStore->validateBlockREQ->transaction_bodies)
    {

        MUTEX_INSPECTOR;
        XTRY;
        auto h = z->getHash();
        auto it = transaction_pool_of_leader.find(h);
        if (it != transaction_pool_of_leader.end())
        {
            transaction_pool_of_leader.erase(it);
            logNode("removed tx %s", base16::encode(h.container).c_str());
        }
        XPASS;
    }
    c_blocks.clear();
    // blockDBStore = nullptr;
    {
        MUTEX_INSPECTOR;
        XTRY;
        do_heart_beat();
        XPASS;
    }
    XPASS;
    return true;
}
bool Node::Service::GetTransactionREQ(const MsgData::GetTransactionREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    if(state_Z!=STATE_NORMAL)
        return true;
    if (CheckState(r->lc->heart_beat.get(), src_node))
        return true;

    resetTimer();
    if (r->lc->heart_beat->node_leader != node_leader_for_client)
        return true;
    if (!root->verify_lider_certificate(r->lc))
    {
        logErr2("if(!verify_lider_certificate(rft.payload_lc,node_leader))");
        return true;
    }
    if (node_leader_for_client != r->lc->heart_beat->node_leader)
    {
        if (isNodeGreaterOrEqual(r->lc->heart_beat->node_leader, node_leader_for_client))
        {
            node_leader_for_client = r->lc->heart_beat->node_leader;
        }
        else
        {
            // logNode("invalid node cert");
            return true;
        }
    }
    sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));

    REF_getter<MsgData::GetTransactionRSP> rsp = new MsgData::GetTransactionRSP;
    for (auto &z : transaction_pool_of_leader)
    {
        rsp->trs.push_back(z.second);
    }
    pass_NodeMsgRSP(rsp.get(), route);
    return true;
}
void Node::Service::pass_NodeMsgRSP(const MsgData::Base *e, const route_t &r)
{
    auto buffer = e->getBuffer();
    auto signature = sign_ed(my_sk_ed, blake2b_hash(buffer).container);
    passEvent(new bcEvent::NodeMsgRSP(this_node_name, signature, buffer, poppedFrontRoute(r)));
}

int get_global_refcount();

bool Node::Service::ValidateBlockREQ(const MsgData::ValidateBlockREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    if (state_Z != State::STATE_NORMAL)
    {
        logErr2("state_Z != State::NORMAL");
        return true;
    }

    t_params t(root);
    bool err = false;
    if (CheckState(r->leader_cert->heart_beat.get(), src_node))
    {
        t.att_data->block_report = {1, "check state heart beat failed"};
        logErr2("check state heart beat failed");
        err = true;
    }
    // return true;

    resetTimer();
    if (!err)
    {
        if (r->leader_cert->heart_beat->node_leader != node_leader_for_client)
        {
            t.att_data->block_report = {1, "cert node leader mismatched"};
            err = true;
            logErr2("cert node leader mismatched");
        }
    }

    if (!err && !root->verify_lider_certificate(r->leader_cert))
    {
        err = true;
        t.att_data->block_report = {1, "verify_lider_certificate failed"};
        logErr2("verify_lider_certificate failed");
    }

    if (!err && r->leader_cert->heart_beat->prev_root_hash != prev_root_hash_Z)
    {
        if (root->getEpoch()->epoch + 1 != r->leader_cert->heart_beat->new_epoch)
        {
            t.att_data->block_report = {1, "epoch invalid"};
            err = true;
            logNode("if (root->getEpoch()->epoch+1 < r->leader_cert->heart_beat->new_epoch)");
            // setBlockId(r->leader_cert->heart_beat->prev_root_hash);
            // return true;
        }
        logNode("ERROR: ValidateBlock block %s, nextblock %s", r->leader_cert->heart_beat->prev_root_hash.str().c_str(), prev_root_hash_Z.str().c_str());
    }
    if (!err)
    {

        // auto new_root_hash =
        t.validateBlockREQ = r;

        auto new_root_hash = execute_block(t, r->leader_cert->nodes);

        auto &c = c_blocks[prev_root_hash_Z];
        if (!c.blockDBStore.valid())
            c.blockDBStore = new MsgData::BlockDBStore;
        c.blockDBStore->validateBlockREQ=r;
        // blockDBStore = prepareBlockDBStore(t);
        c.att_data=t.att_data;

        REF_getter<MsgData::BlockInfo> block = new MsgData::BlockInfo();
        block->prev_root_hash = prev_root_hash_Z;
        block->new_root_hash1 = new_root_hash;

        block->attachment_hash = t.att_data->getHash();
        block->payload_heart_beat = r->leader_cert->heart_beat;

        REF_getter<MsgData::ValidateBlockRSP> rsp = new MsgData::ValidateBlockRSP();
        rsp->node_validator = this_node_name;
        rsp->blockInfo = block;
        rsp->sign(my_sk_bls);

        pass_NodeMsgRSP(rsp.get(), route);
    }
#ifdef MEMLEAK_CHECK
    logNode("!!!!!!!!!!!!!! global REF count %d", get_global_refcount());
    std::vector<std::string> v;
    root->print_calcers(v);
    for (auto &z : v)
    {
        printf("calcer %s\n", z.c_str());
    }
#endif
    return true;
}
