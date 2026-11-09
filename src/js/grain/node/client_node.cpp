#include "NODE_id.h"
#include "REF.h"
#include "blst_cp.h"
#include "blake2bHasher.h"
#include "IUtils.h"
#include "commonError.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <string>
#include <time.h>
#include "md_BlockDBStore.h"
#include "md_ValidateBlockRSP.h"
#include "ioBuffer.h"
#include "nodeService.h"
#include "route_t.h"
#include "s_ed.h"
#include "t_params.h"
#include "tools_mt.h"
#include "QUORUM.h"
#include "cells.h"
#include "proceed_merkle_on_transaction_pool_hashers.h"
#include <vector>
bool Node::Service::BlockDiffValidateREQ(const MsgData::BlockDiffValidateREQ* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    if(!db_state->sync_empty)
    {
        return true;
    }
    stage_is_working=iUtils->getNow();
    auto &cli = cli_leader_info[prev_root_hash_Z()];
    if(!cli.node_leader.valid())
    {
        logNode("if(!cli.node_leader.valid()) 1@");
        return true;
    }

    if(cli.node_leader->node_leader!=src_node)
    {
        logNode("invalid leader 12");
        return true;
    }
    std::vector<blst_cpp::PublicKey> agg_pk;
    for (auto &z : r->blockAcceptedREQ->node_validators)
    {
        XTRY;
        auto n=cells::getNodeNoCreateConst(root.get(),db_state.get(), z);
        if(!n.valid())
            throw CommonError("if(!n.valid()) @09");

        agg_pk.push_back(n->get_bls_pk());
        XPASS;
    }

    {
        MUTEX_INSPECTOR;
        XTRY;
        if (!r->blockAcceptedREQ->agg_sig.verify(agg_pk, r->blockAcceptedREQ->blockInfo->getHash().container))
        {
            logNode("block aggsig not matched");
            return true;
        }
        else
        {
        }
        XPASS;
    }
    /// TODO: проверка стейка
    auto mf=getMetaFull(r->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp);

    int64_t staked=0;
    for(auto &z: r->blockAcceptedREQ->node_validators)
    {
        staked+=mf->getPrevStake(z);
    }
    int64_t fullstake=0;
    for(auto &z: mf->committe_members)
    {
        fullstake+=mf->getPrevStake(z);
    }
    if((staked*100)/fullstake < QUORUM)
    {
        logNode("failed quorum check V");
        return true;
    }

    Rollback roll;
    // _db_to_save db_dump;
/// применяем диффы
    for(auto& z: r->deleted)
    {
        if(z.size()!=32) throw CommonError("if(z.first.size()!=32)");
            THASH_id h;
            h.container=z;
            auto cc = cells::getByDbIdNoCreate(root.get(),db_state.get(), h);
            if (!cc.valid()) { logNode("deleted: cell not found %s", base16::encode(z).c_str()); continue; }
            if (!cc->data.valid()) { logNode("deleted: no data %s", base16::encode(z).c_str()); continue; }            if (!cc->data.valid()) { logNode("deleted: no data %s", base16::encode(z).c_str()); continue; }
            deleteGranule(cc->data.get(),&roll,db_to_save_Z);
    }
    // for(auto &z: db_dump)
    for(auto& z: r->diffs)
    {
        if(z.first.size()!=32) throw CommonError("if(z.first.size()!=32)");
        THASH_id h;
        h.container=z.first;
        auto leaf=cells::replaceLeaf(root.get(),db_state.get(), h, &roll,z.second);
    }


    auto new_root_hash=proceed_merkle_on_transaction_pool_hashers(root,db_to_save_Z);
    if(r->blockAcceptedREQ->blockInfo->new_root_hash1!=new_root_hash)
    {
        logNode("@@ BlockDiffValidateREQ not matched root hash remote %s local %s",r->blockAcceptedREQ->blockInfo->new_root_hash1.str().c_str(),new_root_hash.str().c_str());
        return true;
    }
    REF_getter<MsgData::BlockDiffValidateRSP> bdvrs=new MsgData::BlockDiffValidateRSP;
    bdvrs->node_validator=this_node_name;
    bdvrs->sig.sign(my_sk_bls,r->blockAcceptedREQ->getHash().container);
    bdvrs->blockInfoHash=r->blockAcceptedREQ->blockInfo->getHash();
    pass_NodeMsgRSP(bdvrs.get(),route);
    return true;
}
static void dump_mem(const char* label, JSIRuntime* rt) {
    JSI_RunGC(rt);   // форсируем GC, чтобы мусор не считался
    JSIMemoryUsage s;
    JSI_ComputeMemoryUsage(rt, &s);
    printf("\n[%s]\n", label);
    printf("  malloc_size      = %lld\n", (long long)s.malloc_size);
    printf("  memory_used_size = %lld\n", (long long)s.memory_used_size);
    printf("  obj_count        = %lld\n", (long long)s.obj_count);
    printf("  str_count        = %lld\n", (long long)s.str_count);
    printf("  atom_count       = %lld\n", (long long)s.atom_count);
    printf("  c_func_count     = %lld\n", (long long)s.c_func_count);
    printf("  array_count      = %lld\n", (long long)s.array_count);
    printf("  shape_count      = %lld\n", (long long)s.shape_count);
}
bool Node::Service::BlockDBStore(const MsgData::BlockDBStore* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;

    if(!db_state->sync_empty)
    {
        return true;
    }
    stage_is_working=iUtils->getNow();



    XTRY;
    auto &cli = cli_leader_info[prev_root_hash_Z()];
    if(!cli.node_leader.valid())
    {
        logNode("if(!cli.node_leader.valid()) @2");
        return true;
    }

    if(cli.node_leader->node_leader!=src_node)
    {
        logNode("invalid leader #12");
        return true;
    }

    if (r->blockAccepted2REQ->blockAcceptedREQ->blockInfo->heart_beat->node_leader != src_node)
    {
        logNode("if(blockDBStore->validateBlockREQ->leader_cert->heart_beat->node_leader!=src_node)");
        return true;
    }
    auto mf=getMetaFull(r->blockAccepted2REQ->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp);

    if(!db_state->sync_empty)
    {
        logNode("if(!db_state->sync_empty)");
        return true;
    }

    int64_t stake_val=0;
    std::vector<blst_cpp::PublicKey> agg_pk_v;
    for (auto &z : r->blockAccepted2REQ->blockAcceptedREQ->node_validators)
    {
        XTRY;
        auto n=cells::getNodeNoCreateConst(root.get(),db_state.get(),z);
        if(!n.valid())
            throw CommonError("if(!n.valid()) @86");

        agg_pk_v.push_back(n->get_bls_pk());
        stake_val+=mf->getPrevStake(z);
        XPASS;
    }
    int64_t full_stake_val=0;
    for(auto& z: mf->committe_members)
    {
        full_stake_val+=mf->getPrevStake(z);
    }

    if((stake_val*100)/full_stake_val < QUORUM)
    {
        logNode("validator quorum failed");
        return true;
    }
    {
        MUTEX_INSPECTOR;
        XTRY;
        if (!r->blockAccepted2REQ->blockAcceptedREQ->agg_sig.verify(agg_pk_v, r->blockAccepted2REQ->blockAcceptedREQ->blockInfo->getHash().container))
        {
            logNode("block aggsig not matched");
            return true;
        }
        XPASS;
    }
    int64_t stake_n=0;
    std::vector<blst_cpp::PublicKey> agg_pk_n;
    for(auto & z: r->blockAccepted2REQ->node_diff_validators)
    {
        auto n=cells::getNodeNoCreateConst(root.get(), db_state.get(),z);
        if(!n.valid())
            throw CommonError("if(!n.valid()) ##88");
        stake_n+=n->get_full_stake();
        agg_pk_n.push_back(n->get_bls_pk());
    }
    int64_t full_stake=0;
    auto ls=cells::getAllNodes(root.get(), db_state.get());
    for(auto &z: ls)
    {
        if(!z->isEnabled()) continue;

        full_stake+=z->get_full_stake();
    }
    if((stake_n*100)/full_stake < QUORUM)
    {
        logNode("node diff quorum failed %lld",(stake_n*100)/full_stake);
        return true;
    }
    if (!r->blockAccepted2REQ->agg_diff_sig.verify(agg_pk_n, r->blockAccepted2REQ->blockAcceptedREQ->getHash().container))
    {
        logNode("block diff aggsig not matched");
        return true;
    }



    if(this_node_name==r->blockAccepted2REQ->blockAcceptedREQ->blockInfo->heart_beat->node_leader)
    {
        size_t sz=0;
        for(auto& z:db_to_save_Z.cells)
        {
            sz+=z.second.size();
        }
        logNode("db_state->write_granules_batch %d granules, total size %d",db_to_save_Z.cells.size(),sz);
    }

    db_to_save_Z.add("...last_block...",r->blockAccepted2REQ->getBuffer());


    // stale: читаем старые гранулы ПЕРЕД записью → евент в GrainReader
    std::vector<std::pair<std::string,std::string>> stale;
    for (auto& z : db_to_save_Z.cells) {
        std::string old_body;
        if (db_state->getGranule(z.first, &old_body) == 0 && !old_body.empty() && old_body != z.second)
            stale.push_back({blake2b_hash(old_body).container, old_body});
    }
    for (auto& z : db_to_save_Z.deleted) {
        std::string old_body;
        if (db_state->getGranule(z, &old_body) == 0 && !old_body.empty())
            stale.push_back({blake2b_hash(old_body).container, old_body});
    }

    db_state->write_granules_batch(db_to_save_Z);

    if (!stale.empty())
        sendEvent(ServiceEnum::GrainReader, new bcEvent::StaleGranules(stale, this));

    db_to_save_Z.clear();
    invalidate_services();

    sendEvent(ServiceEnum::BlockStreamer, new bcEvent::StreamBlock(r->blockAccepted2REQ, r->tx_hashes, r->diffs, r->att_data,   this));

    prev_block=r->blockAccepted2REQ;
    l_blocks.clear();
    block_meta_full.clear();

    for (auto &z : r->tx_hashes)
    {

        MUTEX_INSPECTOR;
        XTRY;
        auto it = transaction_pool_of_leader.find(z);
        if (it != transaction_pool_of_leader.end())
        {
            transaction_pool_of_leader.erase(it);
            logNode("removed tx %s", base16::encode(z.container).c_str());
        }
        XPASS;
    }
    auto v=cells::getValuesNoCreateConst(root.get(), db_state.get());
    auto ttl_cache=v->getValue("ttl_cache");
    {
        for(auto& z: contractHost->contract_cache)
        {
            dump_mem("ccc",z.second->rt);
        }
        contractHost->contract_cache.clear();
        root=getRoot(db_state.get(),prev_block);
    }
    cli_leader_info.clear();

    stage_is_working=0;

    if(transaction_pool_of_leader.size())
    {
        do_heart_beat(time(NULL));
    }
    XPASS;
    return true;
}

bool Node::Service::GetTransactionREQ(const MsgData::GetTransactionREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;

    stage_is_working=iUtils->getNow();


    if(!db_state->sync_empty)
    {
        logNode("GetTransaction if(!db_state->sync_empty)");
        return true;
    }

    auto prev_root_hash=prev_root_hash_Z();
    auto & cli=cli_leader_info[prev_root_hash];
    if(!cli.node_leader.valid())
    {
        logNode("if(!cli.node_leader.valid())##4");
        return true;
    }

    if(!cli.node_leader.valid() || cli.node_leader->node_leader!=src_node)
    {
        logNode("GetTransaction invalid leader #14  my %s remote %s", cli.node_leader.valid()?cli.node_leader->node_leader.container.c_str():"", src_node.container.c_str());
        return true;
    }

    REF_getter<MsgData::GetTransactionRSP> rsp = new MsgData::GetTransactionRSP;
    for (auto &z : transaction_pool_of_leader)
    {
        rsp->trs.push_back(z.second);
    }
    rsp->hb_hash=r->hb->getHash();
    pass_NodeMsgRSP(rsp.get(), route);
    return true;
}
void Node::Service::pass_NodeMsgRSP(const MsgData::Base *e, const route_t &r)
{
    MUTEX_INSPECTOR;
    auto buffer = e->getBuffer();
    auto signature = sign_ed(my_sk_ed, blake2b_hash(buffer).container);
    passEvent(new bcEvent::NodeMsgRSP(this_node_name, signature, buffer, poppedFrontRoute(r)));
}
void Node::Service::fix_missed_rounds(const std::set<NODE_id>& node_validators)
{
    auto ls=cells::getAllNodes(root.get(), db_state.get());
    auto v=cells::getValuesNoCreateConst(root.get(), db_state.get());
    auto round_limit=v->getValue("missed_rounds_max");
    for(auto &z: ls)
    {
        if(!z->isEnabled())
            continue;
        if(!node_validators.count(z->getName()))
        {
            int missed=0;
            {
                
                z->missed_rounds++;
                z->answered_rounds = 0;
                missed=z->missed_rounds;
                logNode("missed ++");
            }
            if(missed>round_limit)
            {
                logNode("disable node");
                z->setEnabled(false);
            }
            z->setDirty(NULL);
        }
        else
        {
            z->missed_rounds=0;
            z->answered_rounds++;
            z->setDirty(NULL);
        }
    }
}

int get_global_refcount();

bool Node::Service::ValidateBlockREQ(const MsgData::ValidateBlockREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    stage_is_working=iUtils->getNow();


    if(!db_state->sync_empty)
    {
        return true;
    }
    auto prev_root_hash=prev_root_hash_Z();
    b_params t(db_state.get());
    t.meta=getMetaFull(r->heart_beat->block_timestamp);

    bool err = false;

    auto &cli=cli_leader_info[prev_root_hash];
    if(!cli.node_leader.valid())
    {
        logNode("if(!cli.node_leader.valid())##7");
        return true;
    }
    if(cli.node_leader->node_leader!=src_node)
    {
        logNode("invalid leader #15");
        return true;
    }


    if (!err)
    {
        if (!r->heart_beat->equals(cli_leader_info[r->heart_beat->prev_root_hash_1].node_leader))
        {
            t.emit_block("error", R"({"code":-32602,"error":"cert node leader mismatched"})");
            // t.att_data->block_report = {1, "cert node leader mismatched"};
            err = true;
            logNode("cert node leader mismatched");
        }
    }
    if (!err && r->heart_beat->prev_root_hash_1 != prev_root_hash)
    {

        if (epoch_current() != r->heart_beat->new_epoch)
        {
            t.emit_block("error", R"({"code":-32602,"error":"epoch invalid"})");
            err = true;
            logNode("if (epoch_current() != r->heart_beat->new_epoch)");
        }
        logNode("ERROR: ValidateBlock block %s, nextblock %s", r->heart_beat->prev_root_hash_1.str().c_str(), prev_root_hash_Z().str().c_str());
    }
    if(!err)
    {
        if(!verifyLeaderCert(r->lc.get(),t.meta.get()))
            err="leader cert verify failed";
    }
    if(!err)
        fix_missed_rounds(r->lc->node_validators);
    if (!err)
    {

        MUTEX_INSPECTOR;
        // auto new_root_hash =
        t.validateBlockREQ = r;
        auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
        t.fees = FeesSnapshot{
            .gas_per_token        = v->getValue("gas_per_token"),
            .gas_per_byte         = v->getValue("gas_per_byte"),
            .gas_per_jsi_step     = v->getValue("gas_per_jsi_step"),
            .gas_per_hostcall     = v->getValue("gas_per_hostcall"),
            .gas_per_call         = v->getValue("gas_per_call"),
            .min_tx_fee           = v->getValue("min_tx_fee"),
            .gas_node_create      = v->getValue("gas_node_create"),
            .gas_node_enable      = v->getValue("gas_node_enable"),
            .gas_node_stake       = v->getValue("gas_node_stake"),
            .gas_node_update      = v->getValue("gas_node_update"),
            .gas_node_disable     = v->getValue("gas_node_disable"),
            .gas_contract_deploy  = v->getValue("gas_contract_deploy"),
            .gas_create_base      = v->getValue("gas_create_base"),
            .ttl_seconds_per_gas  = v->getValue("ttl_seconds_per_gas"),
            .gas_create_address   = v->getValue("gas_create_address"),
        };
        auto new_root_hash = execute_block(t, r->heart_beat);
                
        std::map<std::string, std::string> diffs;
        for(auto& z:db_to_save_Z.cells)
        {
            if(z.first.size()==32)
            {
                diffs[z.first]=z.second;
            }
            
        }
        std::set<std::string> deleted;
        for (auto& z : db_to_save_Z.deleted) if (z.size() == 32) deleted.insert(z);

        std::vector<THASH_id> tx_hashes;
        THASH_id tx_hash;
        Blake2bHasher th;
        for(auto & z:r->transaction_bodies)
        {
            auto h=z->getHash();
            tx_hashes.push_back(h);
            th.update(h.container);
        }
        tx_hash.container=th.final();

        THASH_id diff_hash;
        Blake2bHasher dh;
        for(auto &z: diffs)
        {
            dh.update(z.first);
            dh.update(z.second);
        }
        for(auto& z: deleted)
        {
            dh.update(z);
        }
        diff_hash.container=dh.final();


        REF_getter<MsgData::BlockInfo> blockInfo = new MsgData::BlockInfo(new_root_hash,tx_hash,diff_hash,t.att_data->getHash(),r->heart_beat);
        REF_getter<MsgData::ValidateBlockRSP> rsp = new MsgData::ValidateBlockRSP();

        rsp->blockInfo = blockInfo;
        rsp->diffs=diffs;
        rsp->deleted=deleted;
        rsp->tx_hashes=tx_hashes;
        rsp->att_data = t.att_data;

        rsp->node_validator = this_node_name;
        rsp->sign(my_sk_bls);

        pass_NodeMsgRSP(rsp.get(), route);
    }
#ifdef MEMLEAK_CHECK
    logNode("!!!!!!!!!!!!!! global REF count %d", get_global_refcount());
    std::vector<std::string> v;
    // root->print_calcers(v);
    // for (auto &z : v)
    // {
    //     printf("calcer %s\n", z.c_str());
    // }
#endif
    return true;
}
