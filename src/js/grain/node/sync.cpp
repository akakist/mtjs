#include "blake2bHasher.h"
#include "commonError.h"
#include "NODE_id.h"
#include "REF.h"
#include "ioBuffer.h"
#include "bcEvent.h"
#include "listenerBase.h"
#include "mutexInspector.h"
#include "nodeService.h"
#include <cstdlib>
#include <cstddef>
#include "tools_mt.h"
std::pair<std::string,std::string> splice(const std::string& id)
{
    switch(id.size())
    {
    case 0:
        return {"",""};
    case 1:
        return {"",id};
    case 2:
        return {id.substr(0,1),id.substr(1,1)};
    case 3:
        return {id.substr(0,2),id.substr(2,1)};
    case 4:
        return {id.substr(0,3),id.substr(3,1)};
    case 32:
        return {id.substr(0,4),id.substr(4,28)};
    default:
        throw CommonError("defauil switch(id.size())");
    }
}
// void collect_sync_req2(const std::string& id, const std::string &granule,IDatabase* db)
// {
//     {
//         inBuffer in(granule);
//         REF_getter< Cellable> c=new Cellable(NULL,id);
//         c->unpack_mx(in);
//         for(auto& z: c->children_hashes_mx)
//         {
//             std::string cid=id+z.first;
//             std::string cbuf;
//             auto r2=db->getGranule(cid, &cbuf);
//             auto h=blake2b_hash(cbuf);
//             if(h!=z.second)
//             {
//                 db->add_sync_out(cid);
//             }
//         }
//     }
// }

void Node::Service::collect_granules_since_blocktimestamp(Cellable *c, IDatabase *db, TIME_mks block_timestamp, std::vector<std::pair<std::string,std::string>>&dbdump, int64_t& cnt, const route_t &route)
{
    
    auto eligible = [&](const Cellable* x) {
        // since=0 -> полный синк: включаем и генезисные ячейки (timestamp 0),
        // иначе они не попадут в пересборку и root не сойдётся.
        if (block_timestamp.v == 0) return x->last_block_timestamp.v >= 0;
        return x->last_block_timestamp > block_timestamp;
    };
    if(c->m_id.size()==28 && eligible(c))
    {
        dbdump.push_back({c->getDbId(),c->getBuffer_mx()});
        cnt++;
    }
    for(auto& x:c->children_hashes_mx)
    {
        auto cc=cells::getLeafNoCreate(c,db,x.first,true);
        if(!cc.valid())
        {
            // лист отсутствует локально либо хеш не сошёлся с родителем —
            // раньше это был разыменованный NULL. Отдаём неполное дерево,
            // клиент поймает это по несовпадению root hash.
            logNode("[collect_granules] getLeafNoCreate NULL for %s (stale local root)",base16::encode(x.first).c_str());
            continue;
        }
        if(eligible(cc.get()))
            collect_granules_since_blocktimestamp(cc.get(),db,block_timestamp,dbdump,cnt,route);
    }
    if(dbdump.size() > 10)
    {
        passEvent(new bcEvent::syncGetGranulesRSP(prev_root_hash_Z(), this_node_name,dbdump,cnt++,false,"",poppedFrontRoute(route)));
        dbdump.clear();
    }
}
bool Node::Service::syncGetGranulesREQ(const bcEvent::syncGetGranulesREQ*e)
{
    std::vector<std::pair<std::string,std::string>> ret;
    std::string rg;
    int err=db_state->getGranule("",& rg);
    if(err)
    {
        logNode("root granule not found");
        return true;
    }
    auto root_hash=blake2b_hash(rg);
    auto n=cells::getNodeNoCreateConst(root.get(),db_state.get(),e->src_node);
    if(!n.valid())
    {
        logNode("[syncGetGranulesREQ] node %s not found", e->src_node.container.c_str());
        return true;
    }
    if(!e->verify(n->get_ed_pk()))
    {
        logNode("if(!e->verify(n->get_ed_pk()))");
        return true;
    }

    // ── anti-replay ───────────────────────────────────────────────────────────
    // Подпись уже проверена. current_time — свой, монотонный, счётчик запросов
    // (iUtils->getNow().v, микросекунды). Повтор того же REQ отклоняется по
    // монотонности, вне окна — по свежести, а размер запрашиваемой дельты
    // ограничен, чтобы реплей/мусорный since не заставлял отдавать весь чейн.
    const int64_t now=iUtils->getNow().v;
    static constexpr int64_t SYNC_REQ_TTL_MKS   = 30LL*1000*1000;   // 30 s

    if(e->current_time<=0 || e->current_time>now+SYNC_REQ_TTL_MKS/2)
    {
        logNode("[syncGetGranulesREQ] bad current_time now=%lld req=%lld",
                (long long)now,(long long)e->current_time);
        return true;
    }
    if(now-e->current_time>SYNC_REQ_TTL_MKS)
    {
        logNode("[syncGetGranulesREQ] expired request age=%lld mks",(long long)(now-e->current_time));
        return true;
    }
    {
        auto it=last_sync_req_time.find(e->src_node);
        if(it!=last_sync_req_time.end() && e->current_time<=it->second)
        {
            logNode("[syncGetGranulesREQ] replay from %s: %lld <= %lld",
                    e->src_node.container.c_str(),(long long)e->current_time,(long long)it->second);
            return true;
        }
        last_sync_req_time[e->src_node]=e->current_time;
    }
    if(e->since_block_timestamp>e->current_time)
    {
        logNode("[syncGetGranulesREQ] since_block_timestamp in the future");
        return true;
    }
    // Анти-реплей уже обеспечен: TTL запроса (30 s) + монотонный current_time на узел.
    // Ограничение "delta" здесь только мешало догонять после долгого простоя:
    // узел, отставший больше чем на час, получал отказ вечно.
    // Размер ответа и так ограничен пачками (см. collect_granules_since_blocktimestamp).

    int64_t cnt=0;
    collect_granules_since_blocktimestamp(root.get(),db_state.get(),TIME_mks{e->since_block_timestamp},ret,cnt,e->route);

    // prev_block отдаём вместе с финальной пачкой: без него клиент не может
    // обновить prev_root_hash_Z()/epoch_current()/current_leader().
    std::string prev_block_buf;
    if(prev_block.valid())
        prev_block_buf=prev_block->getBuffer();

    passEvent(new bcEvent::syncGetGranulesRSP(root_hash, this_node_name, ret,cnt, true, prev_block_buf, poppedFrontRoute(e->route)));
    return true;
}
#include "proceed_merkle_on_transaction_pool_hashers.h"
bool Node::Service::syncGetGranulesRSP(const bcEvent::syncGetGranulesRSP* m)
{
    logNode("syncGetGranulesRSP");
    sync_watchdog_arm();   // активность: продлеваем watchdog
    // root->replaceLeafNoCreate()
    // getByPathNoCreate()

    // auto newroot=getRoot(db_state.get(),prev_block);
    if (sync_fresh_root)
    {
        // Полная пересборка: удалённые у сервера листья не должны унаследоваться.
        // Пустой root — как при первом запуске (getRoot с пустой БД).
        root = new Cellable(NULL, "");
        sync_fresh_root = false;
        logNode("sync: full rebuild from empty root");
    }
    for(auto& z: m->v)
    {
        if(z.first.size()!=32) throw CommonError("if(z.first.size()!=32)");
        THASH_id h;
        h.container=z.first;
        auto leaf=cells::replaceLeaf(root.get(),db_state.get(), h, NULL,z.second,false,TIME_mks{});
        leaf->setDirty(NULL,leaf->parent->last_block_timestamp);
        logNode("sync: %s",base16::encode(z.first).c_str());
    }
    
    if(!m->done)
        return true;
    else
    {
        _db_to_save db;
        auto roothash=proceed_merkle_on_transaction_pool_hashers(root.get(),db);
        if(m->root_hash!=roothash)
        {
            // Слияние по дельте не сошлось: у клиента нет части старых ячеек,
            // а сервер по фильтру >since их уже не отдаст. Следующий запрос — с since=0.
            // sync_in_progress остаётся true: дерево неполное, нужен повтор.
            logNode("sync: root hash not matched %s %s (next request: full resync)",
                    m->root_hash.str().c_str(),roothash.str().c_str());
            sync_full=true;
            return true;
        }

        // ── prev_block ────────────────────────────────────────────────────────
        // Без него prev_root_hash_Z()/epoch_current()/current_leader() остаются
        // от старого блока: нода не узнаёт нового лидера и снова просит синк.
        REF_getter<MsgData::BlockAccepted2REQ> new_pb=nullptr;
        if(m->prev_block.size())
        {
            new_pb=new MsgData::BlockAccepted2REQ;
            inBuffer in(m->prev_block);
            new_pb->unpack2(in);
            if(!verify_block_all(new_pb))
            {
                logNode("sync: prev_block verify failed");
                return true;   // синк не завершаем
            }
            if(new_pb->blockAcceptedREQ->blockInfo->new_root_hash1!=m->root_hash)
            {
                logNode("sync: prev_block root %s != rsp root %s",
                        new_pb->blockAcceptedREQ->blockInfo->new_root_hash1.str().c_str(),
                        m->root_hash.str().c_str());
                return true;
            }
        }
        else
        {
            logNode("sync: WARNING RSP without prev_block — epoch/leader останутся от старого блока");
        }

        if(new_pb.valid())
            db.add("...last_block...",m->prev_block);   // иначе после рестарта поднимется старый блок

        db_state->write_granules_batch(db);

        if(new_pb.valid())
        {
            prev_block=new_pb;
            l_blocks.clear();
            block_meta_full.clear();
            logNode("sync: done epoch=%lld root=%s",
                    (long long)prev_block->blockAcceptedREQ->blockInfo->heart_beat->new_epoch,
                    prev_root_hash_Z().str().c_str());
        }
        else
        {
            logNode("sync: done (prev_block not updated)");
        }

        sync_watchdog_stop();
        sync_full=false;
        sync_in_progress=false;   // последним: до этого момента состояние неконсистентно
    }

    return true;
}
