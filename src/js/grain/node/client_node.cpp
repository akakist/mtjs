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
#ifdef KALL
bool Node::Service::BlockDiffValidateREQ_sync(const MsgData::BlockDiffValidateREQ* r, const NODE_id & src_node, const route_t& route)
{
    for(auto& z: r->deleted)
    {
        if(z.size()!=32) throw CommonError("if(z.first.size()!=32)");
            THASH_id h;
            h.container=z;
            auto cc = cells::getByDbIdNoCreate(root.get(),db_state.get(), h, false);
            if (!cc.valid()) { logNode("deleted: cell not found %s", base16::encode(z).c_str()); continue; }
            if (!cc->data.valid()) { logNode("deleted: no data %s", base16::encode(z).c_str()); continue; }            
            cc->setDirty__(NULL,r->blockAcceptedREQ->blockInfo->block_timestamp);
            deleteGranule(cc->data.get(),NULL,db_to_save_Z,);
    }
    // for(auto &z: db_dump)
    for(auto& z: r->diffs)
    {
        if(z.first.size()!=32) throw CommonError("if(z.first.size()!=32)");
        THASH_id h;
        h.container=z.first;
        auto leaf=cells::replaceLeaf(root.get(),db_state.get(), h, NULL,z.second,false,r->blockAcceptedREQ->blockInfo->block_timestamp);
        if (leaf.valid()) leaf->setDirty(NULL,r->blockAcceptedREQ->blockInfo->block_timestamp);
    }


    auto new_root_hash=proceed_merkle_on_transaction_pool_hashers(root,db_to_save_Z);

    return true;
}
#endif

// Диагностика: дамп дерева (дети + их хеши + ts + dirty) для сравнения
// состояний лидера и валидатора. Включается MTJS_TREE_DUMP=1.
void Node::Service::dump_tree(Cellable* c, IDatabase* db, const char* tag, int depth, int maxdepth)
{
    if (!c || depth > maxdepth) return;
    std::string buf = c->getBuffer_mx();
    auto h = blake2b_hash(buf);
    std::string ch;
    for (auto& kv : c->children_hashes_mx)
        ch += base16::encode(kv.first).substr(0,8) + "=" +
              base16::encode(kv.second.container).substr(0,8) + " ";
    std::string raw;
    if (buf.size() <= 512) raw = " raw=" + base16::encode(buf);
    logNode("[TREE %s] %*s%s h=%s ts=%lld dirty=%d data=%d ch=[%s]%s",
            tag, depth*2, "",
            base16::encode(c->getDbId()).substr(0,12).c_str(),
            base16::encode(h.container).substr(0,12).c_str(),
            (long long)c->last_block_timestamp.v,
            (int)c->is_dirty, (int)c->data.valid(), ch.c_str(), raw.c_str());
    for (auto& kv : c->children_hashes_mx)
    {
        // Сначала in-memory (children_ptrs_mx): изменённые в текущем блоке
        // ячейки ещё не в БД. Только если указателя нет — грузим из БД.
        REF_getter<Cellable> child;
        auto itp = c->children_ptrs_mx.find(kv.first);
        if (itp != c->children_ptrs_mx.end()) child = itp->second;
        else child = cells::getLeafNoCreate(c, db, kv.first, false);
        if (child.valid()) dump_tree(child.get(), db, tag, depth+1, maxdepth);
        else logNode("[TREE %s] %*s(child %s not loadable)", tag, (depth+1)*2, "",
                     base16::encode(kv.first).substr(0,8).c_str());
    }
}

bool Node::Service::BlockDiffValidateREQ(const MsgData::BlockDiffValidateREQ* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;
    if(sync_in_progress)
    {
        return true;
    }
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
    if(!cli.node_leader->equals(r->blockAcceptedREQ->blockInfo->heart_beat))
    {
        logNode("if(!cli.node_leader->equals(r->blockAcceptedREQ->blockInfo->heart_beat))");
        return true;
    }
    // База блока должна совпадать с нашим root. equals сверяет хартбиты между
    // собой, а не с локальным деревом, поэтому без этой проверки диффы ложились
    // на чужую базу и root никогда не сходился.
    if(r->blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1!=prev_root_hash_Z())
    {
        logNode("BlockDiffValidateREQ: stale base remote %s local %s — ignore",
                r->blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1.str().c_str(),
                prev_root_hash_Z().str().c_str());
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
    auto mf=getMetaFull();

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

    // Порог компакции ДО применения диффов: если в этом блоке он вырос,
    // валидатор обязан применить то же правило чистки, что и лидер.
    TIME_mks old_cutoff;
    {
        auto st0 = cells::getStateNoCreateConst(root.get(), db_state.get());
        if (st0.valid()) old_cutoff = st0->compact_before_ts;
    }

    // _db_to_save db_dump;
/// применяем диффы
    for(auto& z: r->deleted)
    {
        if(z.size()!=32) throw CommonError("if(z.first.size()!=32)");
            THASH_id h;
            h.container=z;
            auto cc = cells::getByDbIdNoCreate(root.get(),db_state.get(), h, true, &roll, r->blockAcceptedREQ->blockInfo->block_timestamp);
            if (!cc.valid()) { logNode("deleted: cell not found %s", base16::encode(z).c_str()); continue; }
            if (!cc->data.valid()) { logNode("deleted: no data %s", base16::encode(z).c_str()); continue; }            
            // WriteGuard gcc(cc, &roll,r->blockAcceptedREQ->blockInfo->block_timestamp,false);
            cc->markDirty(r->blockAcceptedREQ->blockInfo->block_timestamp);
            deleteGranule(cc->data.get(),&roll,db_to_save_Z,r->blockAcceptedREQ->blockInfo->block_timestamp);
    }
    // for(auto &z: db_dump)
    for(auto& z: r->diffs)
    {
        if(z.first.size()!=32) throw CommonError("if(z.first.size()!=32)");
        THASH_id h;
        h.container=z.first;
        auto leaf=cells::replaceLeaf(root.get(),db_state.get(), h, &roll,z.second,true,r->blockAcceptedREQ->blockInfo->block_timestamp);
        if (leaf.valid()) leaf->setDirty(&roll,r->blockAcceptedREQ->blockInfo->block_timestamp);
    }


    // Компакция: если лидер в этом блоке поднял compact_before_ts,
    // каждый валидатор физически чистит тумбстоуны у себя — иначе root не сойдётся.
    {
        auto st = cells::getStateNoCreateConst(root.get(), db_state.get());
        if (st.valid() && st->compact_before_ts > old_cutoff)
        {
            int64_t unlimited = INT64_MAX;
            std::vector<std::string> pruned;
            prune_tombstones(root.get(), db_state.get(), st->compact_before_ts,
                             unlimited, pruned,
                             r->blockAcceptedREQ->blockInfo->block_timestamp, &roll);
            for (auto& id : pruned) local_deletes.push_back(id);
            logNode("BlockDiffValidateREQ: prune by compact_before_ts=%lld removed=%d",
                    (long long)st->compact_before_ts.v, (int)pruned.size());
        }
    }

    // Считаем root в ЛОКАЛЬНЫЙ буфер: при несовпадении нельзя оставлять
    // в общем db_to_save_Z ячейки отравленного дерева — иначе они уедут
    // в БД следующим коммитом.
    _db_to_save db_local;
    auto new_root_hash=proceed_merkle_on_transaction_pool_hashers(root,db_local);
    if(r->blockAcceptedREQ->blockInfo->new_root_hash1!=new_root_hash)
    {
        logNode("@@ BlockDiffValidateREQ not matched root hash remote %s local %s",
                r->blockAcceptedREQ->blockInfo->new_root_hash1.str().c_str(),new_root_hash.str().c_str());
        if (getenv("MTJS_TREE_DUMP")) {
            logNode("[TREE validator] epoch=%d local=%s remote=%s",
                    (int)r->blockAcceptedREQ->blockInfo->heart_beat->new_epoch,
                    new_root_hash.str().c_str(),
                    r->blockAcceptedREQ->blockInfo->new_root_hash1.str().c_str());
            dump_tree(root.get(), db_state.get(), "validator", 0, 6);
        }
        if (getenv("MTJS_TREE_DUMP")) {
            std::string dbg;
            for (auto& z : r->diffs)
                dbg += base16::encode(z.first).substr(0,12) + ":" +
                       base16::encode(blake2b_hash(z.second).container).substr(0,12) + " ";
            logNode("[DIAG validator diff] n=%zu %s", r->diffs.size(), dbg.c_str());
        }
        // Диффы уже применены к root — без отката узел остаётся с затёртым деревом
        // и дальше расходится навсегда. Возвращаем все тронутые ячейки.
        for(auto &z: roll.data)
        {
            inBuffer in(z.second);
            z.first->unpack_mx(in);
            z.first->children_ptrs_mx.clear();   // указатели перечитаются из хешей
        }
        return true;
    }
    db_to_save_Z.add(db_local);   // только после успешной сверки
    REF_getter<MsgData::BlockDiffValidateRSP> bdvrs=new MsgData::BlockDiffValidateRSP;
    bdvrs->node_validator=this_node_name;
    bdvrs->sig34.sign(my_sk_bls,r->blockAcceptedREQ->getHash().container);
    bdvrs->blockAcceptedHash=r->blockAcceptedREQ->getHash();

    // DIAG: what validator signs
    {
        blst_cpp::PublicKey my_pk(my_sk_bls);
        DBG(logNode("[DIAG BlockDiffValidateRSP sign] validator=%s pk=%s sig=%s hash=%s",
            this_node_name.container.c_str(),
            base16::encode(my_pk.serialize()).c_str(),
            base16::encode(bdvrs->sig34.serialize()).c_str(),
            r->blockAcceptedREQ->getHash().str().c_str()));
    }

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

    if(sync_in_progress)
    {
        return true;
    }
    
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
    if(!cli.node_leader->equals(r->blockAccepted2REQ->blockAcceptedREQ->blockInfo->heart_beat))
    {
        logNode("if(!cli.node_leader->equals(r->blockAccepted2REQ->blockAcceptedREQ->blockInfo->heart_beat))");
        return true;
    }


    XTRY;

    if (r->blockAccepted2REQ->blockAcceptedREQ->blockInfo->heart_beat->node_leader != src_node)
    {
        logNode("if(blockDBStore->validateBlockREQ->leader_cert->heart_beat->node_leader!=src_node)");
        return true;
    }
    auto mf=getMetaFull();

    if(sync_in_progress)
    {
        logNode("skipped: sync in progress");
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
    std::set<std::string> vals;
    for(auto & z: r->blockAccepted2REQ->node_diff_validators)
    {
        vals.insert(z.container);
        auto n=cells::getNodeNoCreateConst(root.get(), db_state.get(),z);
        if(!n.valid())
            throw CommonError("if(!n.valid()) ##88");
        stake_n+=n->get_full_stake();
        agg_pk_n.push_back(n->get_bls_pk());
    }
    // logNode("diff validators %s", iUtils->join(" ",vals).c_str());
    int64_t full_stake=0;
    auto ls=cells::getAllNodesConst(root.get(), db_state.get());
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

    int64_t now = iUtils->getNow().v;
    for (auto& z : stale)
        stale_cache[z.first] = { z.second, now };


    db_state->write_granules_batch(db_to_save_Z);
    if (!local_deletes.empty()) {
        int skipped = 0;
        for (auto& id : local_deletes) {
            // Ячейку могли пересоздать в этом же блоке (удаление -> создание):
            // если она есть в батче, удалять её из БД нельзя — иначе живая
            // запись пропадёт и получим "hash not matched".
            if (db_to_save_Z.cells.count(id)) { skipped++; continue; }
            db_state->eraseGranule(id);
        }
        logNode("compact: physically deleted %d cells (skipped rewritten: %d)",
                (int)local_deletes.size() - skipped, skipped);
        local_deletes.clear();
    }
    db_to_save_Z.clear();

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
                DBG(logNode("removed tx %s", base16::encode(z.container).c_str()));
            }
            XPASS;
        }
        auto v=cells::getValuesNoCreateConst(root.get(), db_state.get());
        auto ttl_cache=v->getValue("ttl_cache");
        {
    #ifdef DEBUG
            for(auto& z: contractHost->contract_cache)
            {
                dump_mem("ccc",z.second->rt);
            }
    #endif
            contractHost->contract_cache.clear();
            root=getRoot(db_state.get(),prev_block);
        }


    sendEvent(ServiceEnum::Timer,new timerEvent::StopAlarm(TIMER_BLOCK_EXPIRED,toRef(cli.node_leader->node_leader.container),this));
    cli.node_leader=nullptr;
    XPASS;
    return true;
}

void Node::Service::resetBlockExpired(const NODE_id &n)
{
    sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(TIMER_BLOCK_EXPIRED,toRef(n.container),NULL,5., this));
}

bool Node::Service::GetTxREQ(const MsgData::GetTxREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;

    if(sync_in_progress)
    {
        logNode("GetTxREQ: skipped: sync in progress");
        return true;
    }


    if(r->hb->prev_root_hash_1!=prev_root_hash_Z()
        && (!prev_block.valid() || r->hb->new_epoch > prev_block->blockAcceptedREQ->blockInfo->heart_beat->new_epoch+1))
    {
        // since=0 — базы нет (свежий узел без prev_block) либо предыдущее
        // слияние по дельте не сошлось (sync_full): просим дерево целиком
        const int64_t since = (prev_block.valid() && !sync_full)
            ? prev_block->blockAcceptedREQ->blockInfo->block_timestamp.v : 0;
        if (sync_full) sync_fresh_root = true;   // собираем дерево с нуля
        {
            auto st = cells::getStateNoCreateConst(root.get(), db_state.get());
            sync_old_cutoff = st.valid() ? st->compact_before_ts : TIME_mks{};
        }
        logNode("request sync since=%lld epoch_req=%lld local_epoch=%lld prev_block=%s full=%d",
                (long long)since,(long long)r->hb->new_epoch,(long long)epoch_current(),
                prev_block.valid()?"yes":"no",(int)sync_full);
        REF_getter<bcEvent::syncGetGranulesREQ> e=new bcEvent::syncGetGranulesREQ(since,this_node_name,iUtils->getNow().v,ListenerBase::serviceId);
        e->sign(my_sk_ed);
        sendEvent(r->ip,ServiceEnum::Node,e.get());
        sync_in_progress=true;
        sync_watchdog_arm();
        return true;

    }

    auto prev_root_hash=prev_root_hash_Z();
    auto &cli=cli_leader_info[prev_root_hash_Z()];
    if(r->hb->node_leader!=src_node)
    {
        logNode("if(r->hb->node_leader!=src_node)");
        return true;
    }
    if(cli.node_leader.valid())
    {
        // logNode("if(cli.node_leader.valid()) @@@3");
        return true;
    }
    if(r->hb->prev_root_hash_1!=prev_root_hash)
        return true;   // другой root, но epoch ещё не разошёлся — ждём
    auto cl=current_leader();
    if(!cl.valid())
    {
        logNode("GetTxREQ: no leader in slot frame (NO_LEADER_FRAME) — ignore");
        return true;
    }
    if(cl->getName()!=src_node)
    {
        logNode("GetTxREQ: current_leader=%s != src_node=%s",
                cl->getName().container.c_str(),src_node.container.c_str());
        return true;
    }

    cli.node_leader=r->hb;
    
    resetBlockExpired(r->hb->node_leader);

    REF_getter<MsgData::GetTxRSP> rsp = new MsgData::GetTxRSP;
    for (auto &z : transaction_pool_of_leader)
    {
        rsp->trs.push_back(z.second);
    }
    rsp->hb=r->hb;
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
void Node::Service::fix_missed_rounds(const std::set<NODE_id>& node_validators, Rollback* roll, TIME_mks block_timestamp)
{
    if (getenv("MTJS_TREE_DUMP"))
        logNode("[TRACE fix_missed_rounds] enter validators=%zu", node_validators.size());
    auto ls=cells::getAllNodes(root.get(), db_state.get(), roll, block_timestamp);
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
            z->setDirty(NULL, TIME_mks{block_timestamp});
        }
        else
        {
            z->missed_rounds=0;
            z->answered_rounds++;
            z->setDirty(NULL, TIME_mks{block_timestamp});
        }
        // touch активности узла (TTL). getActivityNoCreate (write) бросает при
        // отсутствии активности — поэтому сначала const-проверка.
        if (cells::getActivityNoCreateConst(root.get(), db_state.get(), z->getDbId()).valid()) {
            auto act = cells::getActivityNoCreate(root.get(), db_state.get(), z->getDbId(), roll, block_timestamp);
            act->update_time(TIME_mks{block_timestamp}, NULL);
        }
    }
    if (getenv("MTJS_TREE_DUMP"))
        logNode("[TRACE fix_missed_rounds] leave");
}

int get_global_refcount();

bool Node::Service::ValidateBlockREQ(const MsgData::ValidateBlockREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;

    // logNode("ValidateBlockREQ from %s",src_node.container.c_str());
    if(sync_in_progress)
    {
        return true;
    }
    auto prev_root_hash=prev_root_hash_Z();
    b_params t(db_state.get());
    t.meta=getMetaFull();

    bool err = false;

    auto &cli=cli_leader_info[prev_root_hash];
    if(!cli.node_leader.valid())
    {
        logNode("if(!cli.node_leader.valid())");
        return true;
    }
    if(!r->heart_beat->equals(cli.node_leader))
    {
        logNode("if(!r->heart_beat->equals(cli.node_leader->node_leader))");
        return true;
    }
    if(r->heart_beat->node_leader!=src_node)
    {
        logNode("if(r->heart_beat->node_leader!=src_node)");
        return true;
    }
    resetBlockExpired(r->heart_beat->node_leader);

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
        if (getenv("MTJS_TREE_DUMP") && !r->transaction_bodies.empty()) {
            logNode("[TREE leader] epoch=%d root=%s", (int)r->heart_beat->new_epoch, new_root_hash.str().c_str());
            dump_tree(root.get(), db_state.get(), "leader", 0, 6);
        }
                
        std::map<std::string, std::string> diffs;
        for(auto& z:db_to_save_Z.cells)
        {
            if(z.first.size()==32)
            {
                diffs[z.first]=z.second;
            }
            
        }
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
        diff_hash.container=dh.final();


        REF_getter<MsgData::BlockInfo> blockInfo = new MsgData::BlockInfo(new_root_hash,tx_hash,diff_hash,t.att_data->getHash(),r->heart_beat, r->block_timestamp.v);
        REF_getter<MsgData::ValidateBlockRSP> rsp = new MsgData::ValidateBlockRSP();

        rsp->blockInfo = blockInfo;
        if (getenv("MTJS_TREE_DUMP")) {
            std::string dbg;
            for (auto& z : diffs)
                dbg += base16::encode(z.first).substr(0,12) + ":" +
                       base16::encode(blake2b_hash(z.second).container).substr(0,12) + " ";
            logNode("[DIAG leader diff] n=%zu %s", diffs.size(), dbg.c_str());
        }
        rsp->diffs=diffs;
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
