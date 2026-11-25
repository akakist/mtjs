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
bool Node::Service::GetTxRSP(const MsgData::GetTxRSP *r, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    MUTEX_INSPECTOR;


    for (auto &z : r->trs)
    {
        THASH_id h = z->getHash();
        logNode("transaction_pool_of_leader.insert({h, z});");
        transaction_pool_of_leader.insert({h, z});
    }
    
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
    if(!bt.block_status.valid())
        throw CommonError("if(!bt.block_status.valid())");
    auto mf=getMetaFull();
    auto pk=mf->getBlsPk(src_node);
    if(!pk)
    {
        logNode("if(!pk) wdswe344");
        return true;
    }
    if(!r->sig34.verify(*pk,r->blockAcceptedHash.container))
    {
        logNode("BlockDiffValidateRSP verify failed");
        return true;
    }
    bt.block_status->rounds[r->round].subblocks[r->subblock].add_accepted_by_noncommitte(r->blockAcceptedHash,src_node,r->sig34);
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
    auto &bt = l_blocks[prev_root_hash_Z()];
    if(!bt.block_status.valid())
        throw CommonError("if(!bt.block_status.valid())");

    auto mf=getMetaFull();
    // auto &n=mf
    auto pk=mf->getBlsPk(r->node_validator);
    if(!pk)
    {
        logNode("!pk @#$#*");
        return true;
    }
    if(!r->verify(*pk))
    {
        logNode("if(!r->verify(*pk))");
        return true;
    }
    bt.block_status->rounds[r->round].subblocks[r->subblock].add_accepted_by_committe(r);
    XPASS;
    return true;
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
    // cert_resolved_epoch = -1;
}

bool Node::Service::UnlockRoundREQ(const MsgData::UnlockRoundREQ *r, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    if (sync_in_progress) return true;
    int64_t total = 0, best = 0;
    // if (!verifySplitProof(r->epoch, r->base_root, r->proof, total, best))
    // {
    //     logNode("UnlockRoundREQ от %s: сплит не доказан (epoch=%lld total=%lld best=%lld) — ignore",
    //             src_node.container.c_str(), (long long)r->epoch, (long long)total, (long long)best);
    //     return true;
    // }
    logNode("UnlockRoundREQ от %s: сплит доказан epoch=%lld total=%lld best=%lld",
            src_node.container.c_str(), (long long)r->epoch, (long long)total, (long long)best);
    unlockRoundLocal(r->epoch, r->base_root);
    return true;
}
