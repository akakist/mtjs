#include "Events/System/timerEvent.h"
#include "NODE_id.h"
#include "REF.h"
#include "bigint.h"
#include "blst_cp.h"
#include "blake2bHasher.h"
#include "IUtils.h"
#include "commonError.h"
#include "base62.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <SQLiteCpp/Statement.h>
#include <string>
#include <time.h>
#include <map>
#include "msg.h"
#include "ioBuffer.h"
#include "nodeService.h"
#include "route_t.h"
#include "s_ed.h"
#include "t_params.h"
#include "tools_mt.h"
#include <SQLiteCpp/Database.h>
#include <vector>

bool Node::Service::BlockAcceptedREQ(const MsgData::BlockAcceptedREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    XTRY;
    if (CheckState(r->leader_certificateZ->heart_beat.get(), src_node))
        return true;
    if (state_Z != State::NORMAL)
        return true;

    resetTimer();
    if (!blockDBStore.valid())
        throw CommonError("if (!blockDBStore.valid())");
    blockDBStore->block_accepted_req = r;
    std::vector<blst_cpp::PublicKey> agg_pk;
    for (auto &z : r->node_validators)
    {
        XTRY;
        agg_pk.push_back(root->getNode(z)->bls_pk);
        XPASS;
    }

    {
        XTRY;
        if (!r->agg_sig.verify(agg_pk, blake2b_hash(r->block_payload->getBuffer()).container))
        {
            logNode("block aggsig not matched");
            // do_heart_beat();
            return true;
        }
        else
        {
            // logNode("block verified OK");
        }
        XPASS;
    }

    {
        XTRY;
        if (!root->verify_lider_certificate(r->leader_certificateZ))
        {
            logNode("leader cert not verified");
            return true;
        }
        XPASS;
    }

    if (src_node != r->leader_certificateZ->heart_beat->node_leader)
    {
        logNode("if(src_node!=hb.node_leader)");
        return true;
    }

    db->write_batch(db_to_save_Z);
    db_to_save_Z.clear();

    {
        XTRY;
        SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        SQLite::Statement insert(dbs, "REPLACE INTO blocks (epoch, prev_root_hash, date, data) VALUES (?, ?, ?, ?)");
        insert.bind(1, blockDBStore->epoch.toString());
        insert.bind(2, base62::encode(blockDBStore->block_accepted_req->leader_certificateZ->heart_beat->prev_block_hash.container));
        insert.bind(3, time(NULL));
        insert.bind(4, base62::encode(blockDBStore->getBuffer()));
        insert.exec();
        XPASS;
    }

    sendEvent(ServiceEnum::BlockStreamer, new bcEvent::StreamBlock(blockDBStore->getBuffer(), this));

    prev_block_hash_Z = r->block_payload->new_root_hash1;
    blocks_leader.clear();
    node_leader_for_client.container.clear();
    sendEvent(ServiceEnum::TxValidator, new bcEvent::InvalidateRoot(this));
    sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::InvalidateRoot(this));
    sendEvent(ServiceEnum::GrainReader, new bcEvent::InvalidateRoot(this));

    {
        XTRY;
        REF_getter<MsgData::BlockAcceptedRSP> br = new MsgData::BlockAcceptedRSP();
        br->new_root_hash = prev_block_hash_Z;
        br->node_signer = this_node_name;
        br->sign(my_sk_bls);

        resetTimer();
        pass_NodeMsgRSP(br.get(), route);
        // auto buffer = br->getBuffer();
        // auto signature = sign_ed(my_sk_ed, buffer);
        // passEvent(new bcEvent::NodeMsgRSP(this_node_name, signature, buffer, poppedFrontRoute(route)));
        XPASS;
    }
    iUtils->getNow();

    logErr2("trs size %d", blockDBStore->att_data->trs.size());
    for (auto &z : blockDBStore->att_data->trs)
    {
        XTRY;
        auto h = z->getHash();
        auto it = transaction_pool_of_leader.find(h);
        if (it != transaction_pool_of_leader.end())
        {
            transaction_pool_of_leader.erase(it);
            logNode("removed tx %s", base62::encode(h.container).c_str());
        }
        XPASS;
    }
    blockDBStore = nullptr;
    {
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
    last_leader_cert = r->lc;
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
    pass_NodeMsgRSP(rsp.get(),route);
    return true;
}
void Node::Service::pass_NodeMsgRSP(const MsgData::Base *e,const route_t& r)
{
    auto buffer = e->getBuffer();
    auto signature = sign_ed(my_sk_ed, blake2b_hash(buffer).container);
    passEvent(new bcEvent::NodeMsgRSP(this_node_name, signature, buffer, poppedFrontRoute(r)));

}

int get_global_refcount();

bool Node::Service::ValidateBlockREQ(const MsgData::ValidateBlockREQ *r, const NODE_id &src_node, const route_t &route)
{
    // if(state_Z!=NORMAL)
    //     return true;
    if (CheckState(r->leader_cert->heart_beat.get(), src_node))
        return true;

    resetTimer();
    MUTEX_INSPECTORS("ValidateBlockREQ");
    if (r->leader_cert->heart_beat->node_leader != node_leader_for_client)
        return true;
    if (state_Z != State::NORMAL)
        return true;

    if (!root->verify_lider_certificate(r->leader_cert))
        throw CommonError("if(!verify_lider_certificate(b.leader_cert))");

    if (r->leader_cert->heart_beat->prev_block_hash != prev_block_hash_Z)
    {
        if (root->getEpoch()->epoch + 1 != r->leader_cert->heart_beat->new_epoch)
        {
            logNode("if (root->getEpoch()->epoch+1 < r->leader_cert->heart_beat->new_epoch)");
            // setBlockId(r->leader_cert->heart_beat->prev_block_hash);
            return true;
        }
        logNode("ERROR: ValidateBlock block %s, nextblock %s", r->leader_cert->heart_beat->prev_block_hash.str().c_str(), prev_block_hash_Z.str().c_str());
    }
    {

        // auto new_root_hash =
        t_params t(root);
        t.att_data->trs = r->transaction_bodies;
        execute_block(t, root, prev_block_hash_Z, r->leader_cert->nodes);
        calc_fee_and_rewards(t, r->leader_cert->nodes);

        auto newEpoch = root->getEpoch();
        newEpoch->epoch += 1;
        newEpoch->setDirty(NULL);

        blockDBStore = prepareBlockDBStore(t);

        auto new_root_hash = proceed_merkle_on_transaction_pool_hashers(root);

        REF_getter<MsgData::BlockInfo> block = new MsgData::BlockInfo();
        block->prev_root_hash = prev_block_hash_Z;
        block->new_root_hash1 = new_root_hash;

        block->attachment_hash = t.att_data->getHash();
        block->payload_heart_beat = r->leader_cert->heart_beat;

        REF_getter<MsgData::ValidateBlockRSP> rsp = new MsgData::ValidateBlockRSP();
        // msg::block_response br;
        rsp->node_validator = this_node_name;
        rsp->payload_block = block;
        rsp->sign(my_sk_bls);

        pass_NodeMsgRSP(rsp.get(),route);
        // msg::node_message_ed nn(rsp->getBuffer(), this_node_name, my_sk_ed);
        // passEvent(new bcEvent::MsgReply(nn.getBuffer(), poppedFrontRoute(route)));
    }
#ifdef MEMLEACK_CHECK
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

