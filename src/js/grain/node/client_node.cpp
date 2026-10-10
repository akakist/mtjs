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
        auto n=db_state->getNodeNoCreate(z);
        if(!n.valid())
            throw CommonError("if(!n.valid())");

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

    uint64_t staked=0;
    for(auto &z: r->blockAcceptedREQ->node_validators)
    {
        staked+=mf->getPrevStake(z);
    }
    uint64_t fullstake=0;
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
    for(auto& z: r->diffs)
    {
        if(z.first.size()!=32) throw CommonError("if(z.first.size()!=32)");
        THASH_id h;
        h.container=z.first;
        auto leaf=db_state->replaceLeaf(h, &roll,z.second);
    }
    auto new_root_hash=proceed_merkle_on_transaction_pool_hashers(db_state->root);
    if(r->blockAcceptedREQ->blockInfo->new_root_hash1!=new_root_hash)
    {
        logNode("@@ BlockDiffValidateREQ not matched root hash remote %s local %s",r->blockAcceptedREQ->blockInfo->new_root_hash1.str().c_str(),new_root_hash.str().c_str());
        return true;
    }
    REF_getter<MsgData::BlockDiffValidateRSP> bdvrs=new MsgData::BlockDiffValidateRSP;
    bdvrs->node_validator=this_node_name;
    bdvrs->sig.sign(my_sk_bls,r->blockAcceptedREQ->getHash().container);
    pass_NodeMsgRSP(bdvrs.get(),route);
    return true;
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

    uint64_t stake_val=0;
    std::vector<blst_cpp::PublicKey> agg_pk_v;
    for (auto &z : r->blockAccepted2REQ->blockAcceptedREQ->node_validators)
    {
        XTRY;
        auto n=db_state->getNodeNoCreateConst(z);
        if(!n.valid())
            throw CommonError("if(!n.valid())");

        agg_pk_v.push_back(n->get_bls_pk());
        stake_val+=mf->getPrevStake(z);
        XPASS;
    }
    uint64_t full_stake_val=0;
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
    uint64_t stake_n=0;
    std::vector<blst_cpp::PublicKey> agg_pk_n;
    for(auto & z: r->blockAccepted2REQ->node_diff_validators)
    {
        auto n=db_state->getNodeNoCreateConst(z);
        stake_n+=n->get_full_stake();
        agg_pk_n.push_back(n->get_bls_pk());
    }
    uint64_t full_stake=0;
    auto ls=db_state->getAllNodes();
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
    db_state->write_granules_batch(db_to_save_Z);

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
            logNode("removed tx %s", base16::encode(z.container).c_str());
        }
        XPASS;
    }
    cli_leader_info.clear();
    v_blocks.clear();

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
    if(v_blocks[prev_root_hash].block_validated)
        return true;
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
    if (!err)
    {

    MUTEX_INSPECTOR;
        // auto new_root_hash =
        t.validateBlockREQ = r;

        auto new_root_hash = execute_block(t, r->heart_beat);

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


        REF_getter<MsgData::BlockInfo> blockInfo = new MsgData::BlockInfo(new_root_hash,tx_hash,diff_hash,t.att_data->getHash(),r->heart_beat);
        REF_getter<MsgData::ValidateBlockRSP> rsp = new MsgData::ValidateBlockRSP();

        rsp->blockInfo = blockInfo;
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
    v_blocks[prev_root_hash].block_validated = true;
    return true;
}
