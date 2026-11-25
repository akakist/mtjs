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

// Диагностика: дамп дерева (дети + их хеши + ts + dirty) для сравнения
// состояний лидера и валидатора. Включается MTJS_TREE_DUMP=1.
void Node::Service::dump_tree(Cellable* c, IDatabase* db, const char* tag, int depth, int maxdepth)
{
    if (!c || depth > maxdepth) return;
    std::string buf = c->getBuffer_mx();
    auto h = blake2b_hash(buf);
    std::string ch;
    for (auto& kv : c->children_hashes)
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
    for (auto& kv : c->children_hashes)
    {
        // Сначала in-memory (children_ptrs_mx): изменённые в текущем блоке
        // ячейки ещё не в БД. Только если указателя нет — грузим из БД.
        REF_getter<Cellable> child;
        auto itp = c->children_ptrs.find(kv.first);
        if (itp != c->children_ptrs.end()) child = itp->second;
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
    // Приём BDV больше не завязан на cli.node_leader: его рассылает не только
    // исходный лидер, но и новый — после сбора 2/3-сертификата (шаг 4).
    // Подлинность проверяется ниже: aggregate-подпись 2/3 комитета за blockInfo
    // и стейк-кворум. Достаточно проверить базу и высоту.
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
    if(r->blockAcceptedREQ->blockInfo->heart_beat->new_epoch != epoch_current())
    {
        logNode("BlockDiffValidateREQ: epoch %lld != current %lld — ignore",
                (long long)r->blockAcceptedREQ->blockInfo->heart_beat->new_epoch,
                (long long)epoch_current());
        return true;
    }
    // Мы уже исполнили и подписали ровно этот блок (наш замок этой высоты).
    // Повторно применять диффы нельзя (double apply), достаточно подтвердить
    // hash BlockAcceptedREQ — это и есть недостающая diff-подпись.
    {
        auto it = my_signed_rsp.find(r->blockAcceptedREQ->blockInfo->heart_beat->new_epoch);
        if (it != my_signed_rsp.end() && it->second.valid()
            && it->second->blockInfo.valid() && it->second->blockInfo->heart_beat.valid()
            && it->second->blockInfo->getHash() == r->blockAcceptedREQ->blockInfo->getHash())
        {
            REF_getter<MsgData::BlockDiffValidateRSP> bdvrs = new MsgData::BlockDiffValidateRSP;
            bdvrs->node_validator = this_node_name;
            bdvrs->sig34.sign(my_sk_bls, r->blockAcceptedREQ->blockInfo->getHash().container);
            bdvrs->blockAcceptedHash = r->blockAcceptedREQ->getHash();
            my_diff_rsp[r->blockAcceptedREQ->blockInfo->heart_beat->new_epoch] = bdvrs;
            // persistCliVote(r->blockAcceptedREQ->blockInfo->heart_beat->new_epoch);
            logNode("BlockDiffValidateREQ: блок уже исполнен epoch=%lld root=%s — подпись без повторного применения",
                    (long long)r->blockAcceptedREQ->blockInfo->heart_beat->new_epoch,
                    it->second->blockInfo->new_root_hash1.str().c_str());
            pass_NodeMsgRSP(bdvrs.get(), route);
            return true;
        }
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

    i128 staked=i128(0);
    for(auto &z: r->blockAcceptedREQ->node_validators)
    {
        staked+=mf->getPrevStake(z);
    }
    i128 fullstake=i128(0);
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
            auto cc = cells::getByDbIdNoCreate(root.get(),db_state.get(), h, true, &roll, r->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp);
            if (!cc.valid()) { logNode("deleted: cell not found %s", base16::encode(z).c_str()); continue; }
            if (!cc->data.valid()) { logNode("deleted: no data %s", base16::encode(z).c_str()); continue; }            
            // WriteGuard gcc(cc, &roll,r->blockAcceptedREQ->blockInfo->block_timestamp,false);
            cc->markDirty(r->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp);
            deleteGranule(cc->data.get(),&roll,db_to_save_Z,r->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp);
    }
    // for(auto &z: db_dump)
    for(auto& z: r->diffs)
    {
        if(z.first.size()!=32) throw CommonError("if(z.first.size()!=32)");
        THASH_id h;
        h.container=z.first;
        auto leaf=cells::replaceLeaf(root.get(),db_state.get(), h, &roll,z.second,true);
        // if (leaf.valid()) leaf->parent->markDirty(r->blockAcceptedREQ->blockInfo->block_timestamp);
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
                             r->blockAcceptedREQ->blockInfo->heart_beat->block_timestamp, &roll);
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
        // Откат по roll.data небезопасен: там СЫРЫЕ Cellable*, а детачнутая
        // ячейка могла быть уже освобождена -> SIGSEGV. Вместо отката
        // перечитываем дерево из БД (db_local не коммитим).
        (void)roll;
        root = getRoot(db_state.get(), prev_block);
        return true;
    }
    db_to_save_Z.add(db_local);   // только после успешной сверки
    cacheBlockPayloads(r->att_data);
    REF_getter<MsgData::BlockDiffValidateRSP> bdvrs=new MsgData::BlockDiffValidateRSP;
    bdvrs->node_validator=this_node_name;
    bdvrs->sig34.sign(my_sk_bls,r->blockAcceptedREQ->blockInfo->getHash().container);
    bdvrs->blockAcceptedHash=r->blockAcceptedREQ->getHash();
    my_diff_rsp[r->blockAcceptedREQ->blockInfo->heart_beat->new_epoch] = bdvrs;
    // persistCliVote(r->blockAcceptedREQ->blockInfo->heart_beat->new_epoch);

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
    
    // Приём финального блока — по сертификату, а не по личности отправителя:
    // BA2 может собрать любой, у кого есть 2/3 (спека: финал = 2/3 подписей).
    // Подлинность проверяется ниже: agg_sig комитета за blockInfo, agg_diff_sig
    // за hash BlockAcceptedREQ и стейк-кворумы.
    const auto& info = r->blockAccepted2REQ->blockAcceptedREQ->blockInfo;
    if (info->heart_beat->prev_root_hash_1 != prev_root_hash_Z())
    {
        logNode("BlockDBStore: stale base remote %s local %s — ignore",
                info->heart_beat->prev_root_hash_1.str().c_str(),
                prev_root_hash_Z().str().c_str());
        return true;
    }
    if (prev_block.valid() && prev_block->blockAcceptedREQ->blockInfo->new_root_hash1 == info->new_root_hash1)
    {
        logNode("BlockDBStore: block %s уже применён — ignore", info->new_root_hash1.str().c_str());
        return true;
    }

    XTRY;

    auto mf=getMetaFull();

    if(sync_in_progress)
    {
        logNode("skipped: sync in progress");
        return true;
    }

    i128 stake_val=i128(0);
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
    i128 full_stake_val=mf->full_stake_all;

    if((stake_val*i128(100))/full_stake_val < QUORUM)
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
    // Один BlockAccepted: подписи apply-стадии уже в blockAcceptedREQ.
    // Отдельного diff-серта больше нет — проверять нечего.


    // Локальное дерево обязано быть на корне блока: BDV мог быть пропущен
    // (sync/дроп), тогда писать BA2 поверх базы нельзя — уйдём в sync.
    {
        auto rh = proceed_merkle_on_transaction_pool_hashers(root, db_to_save_Z);
        if (rh != info->new_root_hash1)
        {
            logNode("BlockDBStore: local root %s != block %s — ignore (нужен sync)",
                    rh.str().c_str(), info->new_root_hash1.str().c_str());
            return true;
        }
    }

    cacheBlockPayloads(r->att_data);

    // Тест-хук: один раз выкинуть att_data из кеша и дотянуть её у подписанта
    // (проверка async_dl: запрос по хешу, ответ, кеш, финалайз).
    if (getenv("MTJS_TEST_DL_FETCH") && !dl_test_done && this_node_name.container == "n0")
    {
        dl_test_done = true;
        THASH_id ah = info->att_data_hash;
        data_cache.erase(ah);
        // data_cache_created.erase(ah);
        std::vector<std::string> srcs;
        for (auto& z : r->blockAccepted2REQ->node_diff_validators)
        {
            if (z == this_node_name) continue;
            auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(), z);
            if (n.valid()) srcs.push_back(n->get_ip());
        }
        if (!srcs.empty())
        {
            logNode("[data-test] fetch att %s from %zu nodes", ah.str().c_str(), srcs.size());
            async_dl({ah}, srcs, [this, ah](bool ok) {
                logNode("[data-test] fetch att %s ok=%d", ah.str().c_str(), (int)ok);
            });
        }
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

    int64_t now = getNow().v;
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
        accepted_signed.clear();
        l_blocks.clear();
        block_meta_full.clear();
        // Шаг 3: высота уехала — сбор сертификата за неё больше не нужен,
        // свои RSP/hb/certs старше предыдущей высоты вычищаем.
        cert_collecting = false;
        // pruneRoundState(epoch_current());
        tx_sent.clear();   // sent живёт до конца блока

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


    // лидер раунда — из заголовка блока (cli.node_leader больше не нужен)
    sendEvent(ServiceEnum::Timer,new timerEvent::StopAlarm(TIMER_BLOCK_EXPIRED,toRef(info->heart_beat->node_leader.container),this));
    cli_leader_info[prev_root_hash_Z()].node_leader=nullptr;
    // pruneRoundState(epoch_current());
    XPASS;
    return true;
}

void Node::Service::pass_NodeMsgRSP(const MsgData::Base *e, const route_t &r)
{
    MUTEX_INSPECTOR;
    auto buffer = e->getBuffer();
    auto signature = sign_ed(my_sk_ed, blake2b_hash(buffer).container);
    passEvent(new bcEvent::NodeMsgRSP(this_node_name, signature, buffer, poppedFrontRoute(r)));
}
void Node::Service::mark_activity(const std::set<NODE_id>& alive_set, Rollback* roll, TIME_mks block_timestamp)
{
    // живость: для нод из подписанного alive-сета (пинг) обновляем
    // и activity (физическое удаление/GC), и last_update (кэш, без чтения activity).
    auto ls=cells::getAllNodes(root.get(), db_state.get(), roll, block_timestamp);
    for(auto &z: ls)
    {
        if(!alive_set.count(z->getName()))
            continue;
        z->last_update = block_timestamp;
        if (cells::getActivityNoCreateConst(root.get(), db_state.get(), z->getDbId()).valid()) {
            auto act = cells::getActivityNoCreate(root.get(), db_state.get(), z->getDbId(), roll, block_timestamp);
            act->update_time(TIME_mks{block_timestamp}, NULL);
        }
    }
}

int get_global_refcount();

void Node::Service::process_offer(const THASH_id & offer, const route_t &route)
{
    auto prev_root_hash=prev_root_hash_Z();
    b_params t(db_state.get());
    t.meta=getMetaFull();
    auto &bt=l_blocks[prev_root_hash];
    // bt.cache[r->getHash()]=r;

    bool err = false;

    // Лидер детерминирован слотом: блок должен прийти от current_leader().
    // cli.node_leader (кэш из GetTxREQ) больше не источник истины — иначе нода,
    // пропустившая GetTxREQ (например, после синка), выпадает из раунда.
    // Достройку блока новым лидером это допускает: src_node — текущий лидер,
    // а hb->node_leader может быть предыдущим (кто изначально предложил).
    auto cl=t.meta->leader();
    {
    }
    if (!err)
    {

        MUTEX_INSPECTOR;
        // auto new_root_hash =
        auto d=cache.find(offer);
        if(d==cache.end())
        {
            logNode("cannot find offer");
            return;
        }
        t.validateBlockREQ = dynamic_cast<MsgData::ValidateBlockREQ*>(d->second.get());
        if(!t.validateBlockREQ.valid())
        {
            logNode("if(!t.validateBlockREQ.valid())");
            return;
        }

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
        auto new_root_hash = execute_block(t, t.validateBlockREQ->heart_beat);
        if (getenv("MTJS_TREE_DUMP") && !t.validateBlockREQ->transaction_bodies.empty()) {
            logNode("[TREE leader] epoch=%d root=%s", (int)t.validateBlockREQ->heart_beat->new_epoch, new_root_hash.str().c_str());
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
        for(auto & z:t.validateBlockREQ->transaction_bodies)
            tx_hashes.push_back(z->getHash());

        // Единый payload: att_data несёт диффы, список tx, эмиты/награды
        // и оффер (hb+ts+тела tx). Её хеш и есть blockInfo.att_data_hash.
        // REF_getter<MsgData::Offer> offer = new MsgData::Offer();
        // offer->hb = t.validateBlockREQ->heart_beat;
        // offer->block_timestamp = t.validateBlockREQ->block_timestamp;
        // offer->txs = r->transaction_bodies;
        t.att_data->tx_hashes = tx_hashes;
        t.att_data->diffs = diffs;
        cache[t.att_data->getHash()]=t.att_data.get();
        cacheBlockPayloads(t.att_data);

        REF_getter<MsgData::BlockInfo> blockInfo = new MsgData::BlockInfo(new_root_hash,t.att_data->getHash(), offer,t.validateBlockREQ->heart_beat);
        blockInfo->offer_hash = offer;   // хеш оффера — в blockInfo
        REF_getter<MsgData::ValidateBlockRSP> rsp = new MsgData::ValidateBlockRSP();

        rsp->blockInfo = blockInfo;
        if (getenv("MTJS_TREE_DUMP")) {
            std::string dbg;
            for (auto& z : diffs)
                dbg += base16::encode(z.first).substr(0,12) + ":" +
                       base16::encode(blake2b_hash(z.second).container).substr(0,12) + " ";
            logNode("[DIAG leader diff] n=%zu %s", diffs.size(), dbg.c_str());
        }
        rsp->att_data = t.att_data;

        // ЗАМОК: не подписываем второй блок на той же высоте.
        if (block_lock.valid() && block_lock->heart_beat.valid()
            && block_lock->heart_beat->new_epoch == t.validateBlockREQ->heart_beat->new_epoch
            && block_lock->new_root_hash1 != new_root_hash)
        {
            logNode("ValidateBlockRSP: REFUSE — уже подписан другой блок на высоте %lld (lock=%s new=%s)",
                    (long long)block_lock->heart_beat->new_epoch,
                    block_lock->new_root_hash1.str().c_str(), new_root_hash.str().c_str());
            return;
        }
        rsp->node_validator = this_node_name;
        rsp->sign(my_sk_bls);
        logNode("ValidateBlockRSP: подписан epoch=%lld root=%s",
                (long long)t.validateBlockREQ->heart_beat->new_epoch, new_root_hash.str().c_str());
        block_lock = blockInfo;

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
    return;

}

bool Node::Service::ValidateBlockREQ(const MsgData::ValidateBlockREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;

    // logNode("ValidateBlockREQ from %s",src_node.container.c_str());
    auto mf=getMetaFull();
    auto cl=mf->leader();
    if(cl.name!=src_node)
    {
        logNode("if(mf->leader!=src_node) @@873");
        return true;
    }
    cache[r->getHash()]=r;
    return true;
}

