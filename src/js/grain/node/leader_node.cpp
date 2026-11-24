#include "NODE_id.h"
#include "blake2bHasher.h"
#include "commonError.h"
#include "blst_cp.h"
#include "REF.h"
#include "corelib/mutexInspector.h"
#include <time.h>
#include "nodeService.h"
#include "QUORUM.h"
#include "route_t.h"
#include <vector>
#include <algorithm>
#include "md/md_BlockDiffValidateREQ.h"
#include "md/md_GetValidateBlockRSPREQ.h"
#include "md/md_GetValidateBlockRSPRSP.h"
#ifdef KALL
bool Node::Service::PingRSP(const MsgData::PingRSP *r, const NODE_id &src_node, const route_t &route)
{
    logNode("@@@ bool Node::Service::PingRSP(const MsgData::PingRSP *r, const NODE_id &src_node, const route_t &route)");
    auto &bt = l_blocks[prev_root_hash_Z()];
    auto h = r->hb->getHash();
    // Перепинг для достройки по сертификату (шаг 4): собираем alive-сет
    // отдельно, штатный путь GetTx для этого hb не запускаем.
    if (cert_alive_hb.valid() && h == cert_alive_hb->getHash())
    {
        cert_alive[h].insert_or_assign(src_node, r);
        return true;
    }
    bt.PingRSP_m[h].insert_or_assign(src_node, r);
    if (bt.is_PingRSP_m_quorum(h, root.get(), db_state.get()))
    {
        // кворум набран — короткое окно для поздних понгов (каждый понг ресетит аларм)
        sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::ALARM_do_request_for_tx, toRef(h.container), r->hb.get(), 0.05, this));
        // logNode("is_PingRSP_m_quorum from %s",src_node.container.c_str());
    }
    return true;
}
#endif
bool Node::Service::GetTxRSP(const MsgData::GetTxRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if(sync_in_progress)
    {
        logNode("GetTransactionRSP skipped: sync in progress");
        return true;
    }


    for (auto &z : r->trs)
    {
        THASH_id h = z->getHash();
        transaction_pool_of_leader.insert({h, z});
    }
    auto &bt = l_blocks[prev_root_hash_Z()];
    // if(!bt.leader_cert_2.valid()) {
    //     logNode("GetTransactionRSP: leader_cert_2 invalid, ignore");
    //     return true;
    // }
    auto h=r->hb->getHash();
    bt.GetTxRSP_m[h].insert_or_assign(src_node,r);
    auto mf=getMetaFull();


    // Кворум GetTx больше не нужен: тх просто добавляются в пул лидера.
    // Дошли до лимита — оффер сразу; иначе короткое окно «дожать» поток.

    sendEvent(ServiceEnum::Timer,
            new timerEvent::ResetAlarm(timers::ALARM_START_BLOCK,
                                        toRef(r->hb->getHash().container), r->hb.get(),
                                        0.1, this));

    // if (transaction_pool_of_leader.size() >= (size_t)MAX_TXS_PER_BLOCK)
    //     armBlockStartAlarm(bt, r->hb, 0.0);
    // else
    //     armBlockStartAlarm(bt, r->hb, (double)TX_SHORT_TIMEOUT_MKS / 1000000.0);
    XPASS;
    return true;
}
bool Node::Service::BlockDiffValidateRSP(const MsgData::BlockDiffValidateRSP* r, const NODE_id & src_node, const route_t& route)
{
    MUTEX_INSPECTOR;

    if(sync_in_progress)
    {
        logNode("ValidateBlockRSP skipped: sync in progress");
        return true;
    }
    auto& bt=l_blocks[prev_root_hash_Z()];
    auto& ba_hash=r->blockAcceptedHash;
    auto br=bt.blockDiffValidateREQ_2[ba_hash];
    if(!br.valid())
    {
        logNode("if(!bt.blockDiffValidateREQ.valid())");
        return true;

    }


    if (br->blockAcceptedREQ->blockInfo->heart_beat->prev_root_hash_1 != prev_root_hash_Z())
    {
        logNode("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash from %s", src_node.container.c_str());
        return true;
    }
    auto nnn=cells::getNodeNoCreateConst(root.get(),db_state.get(),r->node_validator);
    if(!nnn.valid())
        throw CommonError("if(!nnn.valid()) @44");


    // DIAG: what leader verifies
    {
        DBG(logNode("[DIAG BlockDiffValidateRSP verify] validator=%s pk=%s sig=%s hash=%s",
            r->node_validator.container.c_str(),
            base16::encode(nnn->get_bls_pk().serialize()).c_str(),
            base16::encode(r->sig34.serialize()).c_str(),
            br->blockAcceptedREQ->getHash().str().c_str()));
    }
    if(br->blockAcceptedREQ->getHash()!=r->blockAcceptedHash)
    {
        logNode("if(br->blockAcceptedREQ->getHash()!=r->blockAcceptedHash)");
        return true;
    }

    if (!r->sig34.verify(nnn->get_bls_pk(),br->blockAcceptedREQ->blockInfo->getHash().container))
    {
        logNode("[BlockDiffValidateRSP] block response not validated (validator=%s, pk=%s, hash=%s)",
            r->node_validator.container.c_str(),
            base16::encode(nnn->get_bls_pk().serialize()).c_str(),
            br->blockAcceptedREQ->getHash().str().c_str());
        return true;
    }
    // auto h=r->blockInfoHash;
    // logNode("[BlockDiffValidateRSP] blockInfoHash %s from %s",r->blockInfoHash.str().c_str(),src_node.container.c_str());
    {
        DBG(logNode("KALL BDRSP push: r->blockAcceptedHash=%s br->getHash=%s nnn_validator=%s",
                r->blockAcceptedHash.str().c_str(),
                br->blockAcceptedREQ->getHash().str().c_str(),
                r->node_validator.container.c_str()));
    }
    bt.BlockDiffValidateRSP_m[r->blockAcceptedHash].push_back(r);
    DBG(logNode("KALL BDRSP push done: key=%s size=%d",
        r->blockAcceptedHash.str().c_str(),
        (int)bt.BlockDiffValidateRSP_m[r->blockAcceptedHash].size()));


    if(bt.is_BlockDiffValidateRSP_m_quorum(r->blockAcceptedHash,root.get(),db_state.get()))
    {
        MUTEX_INSPECTOR;
        XTRY;
        if (getenv("MTJS_CERT_TRACE"))
            logNode("[cert-trace] BDRSP quorum key=%s map=%zu alive_map=%zu",
                    r->blockAcceptedHash.str().c_str(),
                    bt.BlockDiffValidateRSP_m[r->blockAcceptedHash].size(),
                    bt.PingRSP_m.size());
        // Сливаем подписи apply-стадии в тот же BlockAccepted.
        // Подпись diff-RSP уже за blockInfo; кто уже подписал блок — не дублируем.
        REF_getter<MsgData::BlockAcceptedREQ> merged = new MsgData::BlockAcceptedREQ();
        merged->blockInfo = br->blockAcceptedREQ->blockInfo;
        merged->node_validators = br->blockAcceptedREQ->node_validators;
        merged->agg_sig = br->blockAcceptedREQ->agg_sig;
        std::set<std::string> have;
        std::vector<blst_cpp::PublicKey> pks;
        for (auto& nid : merged->node_validators)
        {
            auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(), nid);
            if (!n.valid()) return true;
            pks.push_back(n->get_bls_pk());
            have.insert(nid.container);
        }
        auto vds = bt.BlockDiffValidateRSP_m[r->blockAcceptedHash];
        std::sort(vds.begin(), vds.end(),
                  [](const REF_getter<MsgData::BlockDiffValidateRSP>& a,
                     const REF_getter<MsgData::BlockDiffValidateRSP>& b) {
                      return a->node_validator.container < b->node_validator.container;
                  });
        for (auto &z : vds)
        {
            if (have.count(z->node_validator.container)) continue;
            auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(), z->node_validator);
            if (!n.valid()) return true;
            pks.push_back(n->get_bls_pk());
            merged->agg_sig.add(z->sig34);
            merged->node_validators.insert(z->node_validator);
            have.insert(z->node_validator.container);
        }
        if (!merged->agg_sig.verify(pks, merged->blockInfo->getHash().container))
        {
            logNode("merged BlockAccepted verify FAIL");
            return true;
        }
        {
            REF_getter<MsgData::BlockStatusREQ> st = new MsgData::BlockStatusREQ();
            st->accepted[merged->blockInfo->getHash()] = merged;
            st->has_offer_hash = true;
            st->offer_hash = merged->blockInfo->offer_hash;
            mergeBlockStatus(st.get(), merged->blockInfo->heart_beat->new_epoch);
            auto mf = getMetaFull();
            if (mf.valid())
                broadcast_MsgEvent_via_broadcaster(st.get(), mf->tree_all_nodes);
            BlockStatusREQ(st.get(), this_node_name, route_t());
        }

        XPASS;
    }



    return true;
}


bool Node::Service::ValidateBlockRSP(const MsgData::ValidateBlockRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;
    if(sync_in_progress)
    {
        logNode("ValidateBlockRSP skipped: sync in progress");
        return true;
    }

    if (r->blockInfo->heart_beat->prev_root_hash_1 != prev_root_hash_Z())
    {
        logNode("ValidateBlockRSP: validated block prev_root_hash not matching with current prev_root_hash from %s", src_node.container.c_str());
        return true;
    }
    auto nnn=cells::getNodeNoCreateConst(root.get(), db_state.get(),r->node_validator);
    if(!nnn.valid())
        throw CommonError("if(!nnn.valid())");
    if (!r->verify(nnn->get_bls_pk()))
    {
        logNode("[ValidateBlockRSP] block response not validated ");
        return true;
    }

    auto &bt = l_blocks[prev_root_hash_Z()];
    auto h=r->blockInfo->getHash();
    auto & bl=l_blocks[prev_root_hash_Z()];
    if(!bl.block_status.valid())
    bl.block_status=new MsgData::BlockStatusREQ;
    if(!bl.block_status->accepted[h].valid())
    {
        bl.block_status->accepted[h]=new MsgData::BlockAcceptedREQ;
    }
    if(!bl.block_status->accepted[h]->node_validators.count(r->node_validator))
    {
        bl.block_status->accepted[h]->agg_sig.add(r->sig);
        bl.block_status->accepted[h]->node_validators.insert(r->node_validator);
    }
    // logNode("blockInfo->getHash() %s from node %s",h.str().c_str(),src_node.container.c_str());
    // bt.ValidateBlockRSP_m[h].push_back(r);
    // auto mf=getMetaFull();
    // if(bt.is_ValidateBlockRSP_m_quorum(h,mf.get()))
    // {
    //     XTRY;
    //     REF_getter<MsgData::BlockAcceptedREQ> ba = new MsgData::BlockAcceptedREQ();
    //     // if (!bt.blockInfo_Z[h].valid())
    //     // {
    //     //     bt.blockInfo_Z[h] = r->blockInfo;
    //     // }
    //     // else if (bt.blockInfo_Z[h]->getBuffer() != r->blockInfo->getBuffer())
    //     //     throw CommonError("else if(bh.block_payload!=r->payload_block)");

    //     ba->blockInfo = r->blockInfo;
    //     std::vector<blst_cpp::PublicKey> agg_pk;
    //     std::set<std::string> nnn;
    //     std::string nv_;
    //     // Детерминированный порядок по NODE_id: от него зависит hash
    //     // BlockAcceptedREQ, а по нему подписывается дифф (шаг 4).
    //     auto vbs = bt.ValidateBlockRSP_m[h];
    //     std::sort(vbs.begin(), vbs.end(),
    //               [](const REF_getter<MsgData::ValidateBlockRSP>& a,
    //                  const REF_getter<MsgData::ValidateBlockRSP>& b) {
    //                   return a->node_validator.container < b->node_validator.container;
    //               });
    //     for (auto &z : vbs)
    //     {
    //         nv_+=z->node_validator.container+" ";
    //         auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(),z->node_validator);
    //         if(!n.valid())
    //             throw CommonError("if(!n.valid()) ^14");
    //         auto st=mf->getPrevStake(z->node_validator);
    //         nv_+=std::to_string(st)+" ";
    //         agg_pk.push_back(n->get_bls_pk());
    //         ba->agg_sig.add(z->sig);
    //         ba->node_validators.push_back(z->node_validator);

    //         nnn.insert(z->node_validator.container);
    //     }
    //     if (ba->agg_sig.verify(agg_pk, ba->blockInfo->getHash().container))
    //     {
    //     }
    //     else
    //     {
    //         logNode("block_accepted verified FAIL !!!!!!!!!!!!!!!!!!!!! val count %d",bt.ValidateBlockRSP_m[h].size());
    //         return true;
    //     }
    //     REF_getter<MsgData::BlockDiffValidateREQ> bdv=new MsgData::BlockDiffValidateREQ(
    //             ba, vbs[0]->att_data->tx_hashes, vbs[0]->att_data->diffs,
    //             std::set<std::string>(), vbs[0]->att_data);
    //     sendEvent(ServiceEnum::Timer,
    //         new timerEvent::ResetAlarm(
    //             TIMER_BlockDiffValidateREQ,
    //             NULL,
    //             bdv.get(),
    //             0.05,
    //             this));

    //     bt.blockDiffValidateREQ_2[bdv->blockAcceptedREQ->getHash()]=bdv;



    //     // logNode("validators %s",iUtils->join(" ",nnn).c_str());


    //     // std::string nodelist;
    //     // for(auto &z: bt.HeartBeatRSP_m[h])
    //     // {
    //     //     nodelist+=z.first.container+" ";
    //     // }
    //     // logNode("hb list %s",nodelist.c_str());
    //     XPASS;
    // }
    XPASS;
    return true;
}


// ── Шаг 3: сбор 2/3-сертификата за высоту (новым лидером) ────────────────
//
// Замок гарантирует: на одной высоте нода подписала максимум один root.
// Значит, опрос комитета по эпохе даёт либо >=2/3 за один root (блок финален,
// шаг 4), либо >=2/3 голосов, разложенных по разным рутам (доказанный сплит,
// шаг 5), либо <2/3 — тогда доказательств нет и замок снимать нельзя.

bool Node::Service::isCurrentLeaderMe()
{
    auto cl = current_leader();
    return cl.valid() && cl->getName() == this_node_name;
}

int64_t Node::Service::certStakeFor(BlockMetaFull* mf, const NODE_id& n)
{
    if (!mf) return 0;
    // Голос засчитывается только члену комитета: иначе подпись ноды вне
    // комитета раздувала бы числитель (знаменатель — стейк комитета).
    if (!mf->committe_members.count(n)) return 0;
    auto it = mf->prev_stakes.find(n);
    return it == mf->prev_stakes.end() ? 0 : it->second;
}

void Node::Service::startCertCollection(int64_t epoch)
{
    cert_collecting = true;
    cert_collect = cert_collection{};
    cert_collect.epoch = epoch;
    cert_collect.base_root = prev_root_hash_Z();
    cert_collect.started_mks = getNow().v;

    // Собственный подписанный RSP — сразу голос, без сети.
    auto it = my_signed_rsp.find(epoch);
    if (it != my_signed_rsp.end() && it->second.valid()
        && it->second->blockInfo.valid() && it->second->blockInfo->heart_beat.valid()
        && it->second->blockInfo->heart_beat->new_epoch == epoch
        && it->second->blockInfo->heart_beat->prev_root_hash_1 == cert_collect.base_root)
    {
        cert_collect.votes[this_node_name] = it->second;
    }

    logNode("cert: start epoch=%lld base=%s own_vote=%d",
            (long long)epoch, cert_collect.base_root.str().c_str(),
            (int)cert_collect.votes.size());
    broadcastCertReq();
    sendEvent(ServiceEnum::Timer,
              new timerEvent::ResetAlarm(ALARM_GetValidateBlockRSP, NULL, NULL, 0.5, this));
}

void Node::Service::broadcastCertReq()
{
    if (!cert_collecting) return;
    int64_t now = getNow().v;
    // ре-броадкаст не чаще раза в секунду; 0.5s-аларм продолжает пересчёт
    if (cert_collect.last_broadcast_mks && now - cert_collect.last_broadcast_mks < 1000000)
        return;
    cert_collect.last_broadcast_mks = now;

    REF_getter<MsgData::GetValidateBlockRSPREQ> req = new MsgData::GetValidateBlockRSPREQ();
    req->epoch = cert_collect.epoch;
    req->base_root = cert_collect.base_root;
    if (block_lock.valid() && block_lock->heart_beat.valid()
        && block_lock->heart_beat->new_epoch == cert_collect.epoch)
        req->root_hint = block_lock->new_root_hash1;

    REF_getter<BlockMetaFull> mf;
    try { mf = getMetaFull(cert_collect.base_root); } catch (...) { return; }
    if (!mf.valid()) return;
    broadcast_MsgEvent_via_broadcaster(req.get(), mf->tree_committe);
}

void Node::Service::finishCertCollection()
{
    if (!cert_collecting) return;
    REF_getter<BlockMetaFull> mf;
    try { mf = getMetaFull(cert_collect.base_root); } catch (...) { return; }
    if (!mf.valid()) return;

    // Вариант = blockInfo (подписи — за его хеш), а не корень состояния:
    // два разных blockInfo могут дать один корень (разные диффы), и склейка
    // по корню делает агрегат невалидным.
    std::map<THASH_id, int64_t> stake_by_block;
    std::map<THASH_id, std::vector<REF_getter<MsgData::ValidateBlockRSP>>> rsps_by_block;
    int64_t total = 0;
    for (auto& z : cert_collect.votes)
    {
        int64_t st = certStakeFor(mf.get(), z.first);
        if (st <= 0) continue;
        if (!z.second.valid() || !z.second->blockInfo.valid()) continue;
        const THASH_id bhash = z.second->blockInfo->getHash();
        stake_by_block[bhash] += st;
        rsps_by_block[bhash].push_back(z.second);
        total += st;
    }
    int64_t full = 0;
    for (auto& z : mf->committe_members)
    {
        auto it = mf->prev_stakes.find(z);
        if (it != mf->prev_stakes.end()) full += it->second;
    }
    if (full <= 0) return;

    THASH_id best_block;
    int64_t best_stake = 0;
    for (auto& z : stake_by_block)
        if (z.second > best_stake) { best_stake = z.second; best_block = z.first; }

    const bool root_quorum  = (best_stake * 100) / full > QUORUM;
    const bool total_quorum = (total * 100) / full > QUORUM;

    if (root_quorum)
    {
        // Детерминированный порядок: сортировка по NODE_id. От порядка зависит
        // hash BlockAcceptedREQ, а по нему подписывается дифф (шаг 4) — иначе
        // сертификат не переносится между лидерами.
        auto v = rsps_by_block[best_block];
        std::sort(v.begin(), v.end(),
                  [](const REF_getter<MsgData::ValidateBlockRSP>& a,
                     const REF_getter<MsgData::ValidateBlockRSP>& b) {
                      return a->node_validator.container < b->node_validator.container;
                  });
        cert_entry ce;
        ce.epoch = cert_collect.epoch;
        ce.root = v[0]->blockInfo->new_root_hash1;
        ce.stake = best_stake;
        ce.rsps = v;
        certs[best_block] = ce;

        std::string signers;
        for (auto& z : v) signers += z->node_validator.container + " ";
        logNode("cert: COMPLETE epoch=%lld block=%s root=%s stake=%lld/%lld signers=[%s]",
                (long long)ce.epoch, best_block.str().c_str(), ce.root.str().c_str(),
                (long long)best_stake, (long long)full, signers.c_str());
        cert_resolved_epoch = ce.epoch;
        cert_collecting = false;
        sendEvent(ServiceEnum::Timer,
                  new timerEvent::StopAlarm(ALARM_GetValidateBlockRSP, NULL, this));
        startCertFinalize(ce);   // шаг 4: BA -> BDV -> BA2 -> применение
        return;
    }

    if (total_quorum)
    {
        // >=2/3 голосов, но ни один root не набрал 2/3 — доказанный сплит.
        // Замок снимается: рассылаем доказательство и откатываем дерево.
        std::string roots;
        for (auto& z : stake_by_block)
            roots += z.first.str().substr(0, 8) + "=" + std::to_string(z.second) + " ";
        const int64_t E = cert_collect.epoch;
        const THASH_id base = cert_collect.base_root;
        logNode("cert: SPLIT epoch=%lld total=%lld/%lld roots=[%s] — снимаю замок",
                (long long)E, (long long)total, (long long)full, roots.c_str());

        // доказательство = все собранные голоса (отсортированы по NODE_id)
        std::vector<REF_getter<MsgData::ValidateBlockRSP>> proof;
        for (auto& z : cert_collect.votes)
            proof.push_back(z.second);
        std::sort(proof.begin(), proof.end(),
                  [](const REF_getter<MsgData::ValidateBlockRSP>& a,
                     const REF_getter<MsgData::ValidateBlockRSP>& b) {
                      return a->node_validator.container < b->node_validator.container;
                  });
        cert_resolved_epoch = E;
        cert_collecting = false;
        sendEvent(ServiceEnum::Timer,
                  new timerEvent::StopAlarm(ALARM_GetValidateBlockRSP, NULL, this));

        unlockRoundLocal(E, base);
        {
            REF_getter<MsgData::UnlockRoundREQ> req = new MsgData::UnlockRoundREQ();
            req->epoch = E;
            req->base_root = base;
            req->proof = proof;
            REF_getter<BlockMetaFull> mf;
            try { mf = getMetaFull(base); } catch (...) { mf = nullptr; }
            if (mf.valid())
                broadcast_MsgEvent_via_broadcaster(req.get(), mf->tree_all_nodes);
        }

        // новый блок на той же высоте: раунд стартует заново с нашим hb
        if (isCurrentLeaderMe())
        {
            l_blocks[prev_root_hash_Z()] = block_leader{};
            logNode("UNLOCK: перезапускаю раунд epoch=%lld новым блоком", (long long)E);
            sendEvent(ServiceEnum::Timer,
                      new timerEvent::ResetAlarm(ALARM_Ping, NULL, NULL, 0.2, this));
        }
        return;
    }

    // < 2/3 — неизвестно: стоим, замок не снимаем.
    logNode("cert: waiting epoch=%lld votes=%zu stake=%lld/%lld best=%s/%lld",
            (long long)cert_collect.epoch, cert_collect.votes.size(),
            (long long)total, (long long)full,
            best_block.str().c_str(), (long long)best_stake);
}

bool Node::Service::maybeCollectCert()
{
    if (sync_in_progress || !prev_block.valid()) return false;
    const int64_t E = epoch_current();

    if (!isCurrentLeaderMe())
    {
        if (cert_collecting)
        {
            logNode("cert: not leader anymore — stop epoch=%lld", (long long)cert_collect.epoch);
            cert_collecting = false;
        }
        return false;
    }

    if (cert_collecting)
    {
        if (cert_collect.epoch != E)
        {
            // высота уехала (блок применили) — сбор больше не нужен
            cert_collecting = false;
            sendEvent(ServiceEnum::Timer,
                      new timerEvent::StopAlarm(ALARM_GetValidateBlockRSP, NULL, this));
            return false;
        }
        broadcastCertReq();
        return true;
    }

    // По этой высоте сбор уже завершён (сертификат или сплит-пруф) —
    // повторно не собираем; шаг 4/5 действует по сохранённому результату.
    if (cert_resolved_epoch == E)
        return true;

    // Признак «на этой высоте уже подписан блок» — наш собственный замок,
    // подписанный у ДРУГОГО лидера. Только он: если мы не подписывали,
    // то и доказательств у нас нет, а предложить новый блок безопасно —
    // подписанты старого откажут по замку, и 2/3 за конфликт не соберётся.
    const bool pending =
        block_lock.valid() && block_lock->heart_beat.valid()
        && block_lock->heart_beat->new_epoch == E
        && block_lock->heart_beat->node_leader != this_node_name
        && block_lock->new_root_hash1 != prev_root_hash_Z();
    if (!pending) return false;

    startCertCollection(E);
    return true;
}

bool Node::Service::GetValidateBlockRSPRSP(const MsgData::GetValidateBlockRSPRSP *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    if (!cert_collecting) return true;
    if (r->epoch != cert_collect.epoch) return true;

    for (auto& z : r->rsps)
    {
        if (!z.valid() || !z->blockInfo.valid() || !z->blockInfo->heart_beat.valid())
            continue;
        // ответчик отвечает только за себя
        if (z->node_validator != src_node)
        {
            logNode("cert: rsp validator %s != sender %s — ignore",
                    z->node_validator.container.c_str(), src_node.container.c_str());
            continue;
        }
        if (z->blockInfo->heart_beat->new_epoch != cert_collect.epoch) continue;
        if (z->blockInfo->heart_beat->prev_root_hash_1 != cert_collect.base_root) continue;

        auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(), z->node_validator);
        if (!n.valid()) continue;
        if (!z->verify(n->get_bls_pk()))
        {
            logNode("cert: bad BLS sig from %s — ignore", src_node.container.c_str());
            continue;
        }
        auto it = cert_collect.votes.find(src_node);
        if (it != cert_collect.votes.end())
        {
            if (it->second->blockInfo->new_root_hash1 == z->blockInfo->new_root_hash1)
                continue;   // дубль
            logNode("cert: %s voted twice (lock broken?) — keep first", src_node.container.c_str());
            continue;
        }
        logNode("cert: vote from %s root=%s epoch=%lld",
                src_node.container.c_str(), z->blockInfo->new_root_hash1.str().c_str(),
                (long long)cert_collect.epoch);
        cert_collect.votes[src_node] = z;
    }

    finishCertCollection();
    return true;
}

// ── Шаг 4: достройка блока по сертификату ────────────────────────────────
// Из отсортированных по NODE_id RSP собираем канонический BlockAcceptedREQ
// (hash детерминирован), публикуем его со стадией диффа и даём штатному
// пути TIMER_BlockDiffValidateREQ -> BlockDiffValidateRSP -> BlockDBStore
// довести блок до применения.
void Node::Service::startCertFinalize(const cert_entry &ce)
{
    MUTEX_INSPECTOR;
    if (getenv("MTJS_CERT_TRACE"))
        logNode("[cert-trace] FINALIZE check epoch=%lld cur=%lld sync=%d prev=%d rsps=%zu",
                (long long)ce.epoch, (long long)epoch_current(), (int)sync_in_progress,
                (int)prev_block.valid(), ce.rsps.size());
    if (sync_in_progress || !prev_block.valid()) return;
    if (ce.epoch != epoch_current()) return;
    if (ce.rsps.empty()) return;

    const auto& body = ce.rsps[0];
    if (!body.valid() || !body->blockInfo.valid() || !body->blockInfo->heart_beat.valid()) return;
    if (body->blockInfo->new_root_hash1 != ce.root) return;

    // Тело должно быть одинаковым у всех подписантов: иначе это подписи за
    // разные блоки, а не сертификат за один root.
    for (auto& z : ce.rsps)
    {
        if (!z.valid() || !z->blockInfo.valid()) return;
        if (z->blockInfo->new_root_hash1 != ce.root) return;
        if (!z->att_data.valid() || z->att_data->getHash() != z->blockInfo->att_data_hash)
        {
            if (getenv("MTJS_CERT_TRACE"))
                logNode("[cert-trace] FINALIZE skip: att_data hash не сходится (%s)",
                        z->node_validator.container.c_str());
            return;
        }
        if (z->blockInfo->getHash() != body->blockInfo->getHash()
            || z->att_data->getHash() != body->att_data->getHash())
        {
            if (getenv("MTJS_CERT_TRACE"))
                logNode("[cert-trace] FINALIZE skip: body differs (%s vs %s)",
                        z->node_validator.container.c_str(), body->node_validator.container.c_str());
            return;
        }
    }

    REF_getter<MsgData::BlockAcceptedREQ> ba = new MsgData::BlockAcceptedREQ();
    ba->blockInfo = body->blockInfo;
    std::vector<blst_cpp::PublicKey> agg_pk;
    for (auto& z : ce.rsps)
    {
        auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(), z->node_validator);
        if (!n.valid()) return;
        agg_pk.push_back(n->get_bls_pk());
        ba->agg_sig.add(z->sig);
        ba->node_validators.insert(z->node_validator);
    }
    if (!ba->agg_sig.verify(agg_pk, ba->blockInfo->getHash().container))
    {
        logNode("cert: FINALIZE отменена — aggregate signature verify FAIL");
        return;
    }

    // Ретранслируем block cert всей сети (MsgData broadcast).
    relayBlockStatus(ba, nullptr);

    auto &bt = l_blocks[prev_root_hash_Z()];
    // Штатный TIMER_BlockDiffValidateREQ проверяет кворум по этой карте.
    bt.ValidateBlockRSP_m[ba->blockInfo->getHash()] = ce.rsps;
    REF_getter<MsgData::BlockDiffValidateREQ> bdv =
        new MsgData::BlockDiffValidateREQ(ba, body->att_data->tx_hashes,
                                          body->att_data->diffs,
                                          std::set<std::string>(), body->att_data);
    bt.blockDiffValidateREQ_2[ba->getHash()] = bdv;

    // alive-сет для BA2: перепингываем hb достраиваемого блока (подпись пинга —
    // над hash hb). Свою подпись кладём сразу, чтобы aggregate не был пустым.
    {
        REF_getter<BlockMetaFull> mf;
        try { mf = getMetaFull(prev_root_hash_Z()); } catch (...) { mf = nullptr; }
        cert_alive.clear();
        cert_alive_hb = body->blockInfo->heart_beat;
        REF_getter<MsgData::PingRSP> self = new MsgData::PingRSP;
        self->hb = cert_alive_hb;
        self->sig.sign(my_sk_bls, cert_alive_hb->getHash().container);
        cert_alive[cert_alive_hb->getHash()][this_node_name] = self;
        if (mf.valid())
        {
            REF_getter<MsgData::PingREQ> ping = new MsgData::PingREQ();
            ping->hb = cert_alive_hb;
            broadcast_MsgEvent_via_broadcaster(ping.get(), mf->tree_all_nodes);
        }
    }

    logNode("cert: FINALIZE epoch=%lld root=%s validators=%zu — запускаю BDV",
            (long long)ce.epoch, ce.root.str().c_str(), ce.rsps.size());
    sendEvent(ServiceEnum::Timer,
              new timerEvent::ResetAlarm(TIMER_BlockDiffValidateREQ, NULL, bdv.get(), 0.05, this));
}

// ── Шаг 5: сплит-пруф и снятие замка ─────────────────────────────────────
// Условие снятия: >=2/3 комитета проголосовали (по стейку), но ни один root
// не набрал 2/3. Тогда кворума за один блок нет, а значит и финального блока
// нет — замок снимается, на этой же высоте предлагается новый блок.
bool Node::Service::verifySplitProof(int64_t epoch, const THASH_id& base,
                                     const std::vector<REF_getter<MsgData::ValidateBlockRSP>>& proof,
                                     int64_t& total_out, int64_t& best_out)
{
    total_out = 0;
    best_out = 0;
    if (epoch != epoch_current()) return false;
    if (base != prev_root_hash_Z()) return false;
    REF_getter<BlockMetaFull> mf;
    try { mf = getMetaFull(base); } catch (...) { return false; }
    if (!mf.valid()) return false;

    int64_t full = 0;
    for (auto& z : mf->committe_members)
    {
        auto it = mf->prev_stakes.find(z);
        if (it != mf->prev_stakes.end()) full += it->second;
    }
    if (full <= 0) return false;

    std::map<NODE_id, REF_getter<MsgData::ValidateBlockRSP>> votes;
    for (auto& z : proof)
    {
        if (!z.valid() || !z->blockInfo.valid() || !z->blockInfo->heart_beat.valid()) continue;
        if (z->blockInfo->heart_beat->new_epoch != epoch) continue;
        if (z->blockInfo->heart_beat->prev_root_hash_1 != base) continue;
        auto n = cells::getNodeNoCreateConst(root.get(), db_state.get(), z->node_validator);
        if (!n.valid()) continue;
        if (!z->verify(n->get_bls_pk())) continue;
        votes.insert_or_assign(z->node_validator, z);   // одна нода — один голос
    }

    std::map<THASH_id, int64_t> by_block;
    int64_t total = 0;
    for (auto& z : votes)
    {
        int64_t st = certStakeFor(mf.get(), z.first);
        if (st <= 0) continue;
        total += st;
        by_block[z.second->blockInfo->getHash()] += st;
    }
    int64_t best = 0;
    for (auto& z : by_block)
        if (z.second > best) best = z.second;

    total_out = total;
    best_out = best;
    return (total * 100) / full > QUORUM && (best * 100) / full <= QUORUM;
}

void Node::Service::unlockRoundLocal(int64_t epoch, const THASH_id& base)
{
    if (epoch != epoch_current()) return;
    if (base != prev_root_hash_Z()) return;
    const bool have_lock = block_lock.valid() && block_lock->heart_beat.valid()
        && block_lock->heart_beat->new_epoch == epoch;
    if (!have_lock) return;

    logNode("UNLOCK: сплит доказан epoch=%lld base=%s lock_root=%s — снимаю замок",
            (long long)epoch, base.str().c_str(), block_lock->new_root_hash1.str().c_str());

    // Наш исполненный, но не финализированный блок не должен оставаться
    // в дереве: откатываемся к корню из БД (применённому состоянию).
    db_to_save_Z.clear();
    local_deletes.clear();
    root = getRoot(db_state.get(), prev_block);
    block_lock = nullptr;
    my_signed_rsp.erase(epoch);
    my_diff_rsp.erase(epoch);
    erase_cli_vote(db_state.get());
    l_blocks[prev_root_hash_Z()] = block_leader{};
    cert_resolved_epoch = -1;
}

bool Node::Service::UnlockRoundREQ(const MsgData::UnlockRoundREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    if (sync_in_progress) return true;
    int64_t total = 0, best = 0;
    if (!verifySplitProof(r->epoch, r->base_root, r->proof, total, best))
    {
        logNode("UnlockRoundREQ от %s: сплит не доказан (epoch=%lld total=%lld best=%lld) — ignore",
                src_node.container.c_str(), (long long)r->epoch, (long long)total, (long long)best);
        return true;
    }
    logNode("UnlockRoundREQ от %s: сплит доказан epoch=%lld total=%lld best=%lld",
            src_node.container.c_str(), (long long)r->epoch, (long long)total, (long long)best);
    unlockRoundLocal(r->epoch, r->base_root);
    return true;
}
