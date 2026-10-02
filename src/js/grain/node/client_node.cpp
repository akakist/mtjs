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

bool Node::Service::BlockValidatedREQ(const MsgData::BlockValidatedREQ *r, const NODE_id &src_node, const route_t &route)
{
        // logErr2("@@ %s",__func__);

    if(!db_state->sync_empty)
    {
        return true;
    }
    // if(state_Z==STATE_SYNCING)
     stage_is_working=iUtils->getNow();
         

   MUTEX_INSPECTOR;

    XTRY;
    auto &cli = cli_leader_info[prev_root_hash_Z()];
    if(!cli.node_leader.valid())
    {
        logNode("if(!cli.node_leader.valid())");
        return true;
    }

    if(cli.node_leader->node_leader!=src_node)
    {
        logNode("invalid leader 12");
        return true;
    }
    auto& v=v_blocks[r->blockInfo->heart_beat->prev_root_hash_1];

    if (!v.blockDBStore.valid())
    {
        logNode("if (!v.blockDBStore.valid())");
        return true;

    }

    if (v.blockDBStore->hb->node_leader != src_node)
    {
        logNode("if(blockDBStore->validateBlockREQ->leader_cert->heart_beat->node_leader!=src_node)");
        return true;
    }

    if(!db_state->sync_empty)
    {
        return true;
    }

    if (! v.blockDBStore.valid())
        throw CommonError("if (!blockDBStore.valid())");
    v.blockDBStore->blockAcceptedREQ = r;
    std::vector<blst_cpp::PublicKey> agg_pk;
    for (auto &z : r->node_validators)
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
        if (!r->agg_sig.verify(agg_pk, blake2b_hash(r->blockInfo->getBuffer()).container))
        {
            logNode("block aggsig not matched");
            return true;
        }
        else
        {
        }
        XPASS;
    }
    if(this_node_name==v.blockDBStore->hb->node_leader)
    {
        size_t sz=0;
        for(auto& z:db_to_save_Z.cells)
        {
            sz+=z.second.size();
        }
        logNode("db_state->write_granules_batch %d granules, total size %d",db_to_save_Z.cells.size(),sz);
    }
    db_to_save_Z.add(".last_block",r->getBuffer());
    // auto &hb=v.blockDBStore->hb;
    // {
    //     MUTEX_INSPECTOR;
    //     XTRY;
    //     XPASS;
    // }
    db_state->write_granules_batch(db_to_save_Z);

    // FILE *f= fopen("")
    logErr2("written %d granules",db_to_save_Z.cells.size());
    db_to_save_Z.clear();


    sendEvent(ServiceEnum::BlockStreamer, new bcEvent::StreamBlock(v.blockDBStore, v.att_data_Z, this));

    prev_block=r;
    l_blocks.clear();
    block_meta_full.clear();
    // block_meta_validator.clear();
    cli_leader_info.clear();

    for (auto &z : v.blockDBStore->tx_hashes)
    {

        MUTEX_INSPECTOR;
        XTRY;
        // auto h = z->getHash();
        auto it = transaction_pool_of_leader.find(z);
        if (it != transaction_pool_of_leader.end())
        {
            transaction_pool_of_leader.erase(it);
            logNode("removed tx %s", base16::encode(z.container).c_str());
        }
        XPASS;
    }
    v_blocks.clear();

    stage_is_working=0;

    if(transaction_pool_of_leader.size())
    {
        // auto mf=getMetaFull(time(NULL));
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

//    if(state_Z==STATE_SYNCING)
//     {
//         
//         return true;
//     }
    auto prev_root_hash=prev_root_hash_Z();
    auto & cli=cli_leader_info[prev_root_hash];
    if(!cli.node_leader.valid())
    {
        logNode("if(!cli.node_leader.valid())");
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
    bool err = false;
    
    auto &cli=cli_leader_info[prev_root_hash];
    if(!cli.node_leader.valid())
    {
        logNode("if(!cli.node_leader.valid())");
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
            // setBlockId(r->leader_cert->heart_beat->prev_root_hash);
            // return true;
        }
        logNode("ERROR: ValidateBlock block %s, nextblock %s", r->heart_beat->prev_root_hash_1.str().c_str(), prev_root_hash_Z().str().c_str());
    }
    if (!err)
    {

        // auto new_root_hash =
        t.validateBlockREQ = r;

        auto new_root_hash = execute_block(t, r->heart_beat);

        auto &v = v_blocks[prev_root_hash_Z()];
        if (!v.blockDBStore.valid())
            v.blockDBStore = new MsgData::BlockDBStore;
        // v.blockDBStore->validateBlockREQ_Z=r;
        v.blockDBStore->hb=r->heart_beat;
        v.blockDBStore->tx_hashes.clear();
        for(auto &z: r->transaction_bodies)
        {
            v.blockDBStore->tx_hashes.push_back(z->getHash());
        }
        // blockDBStore = prepareBlockDBStore(t);
        v.att_data_Z=t.att_data;
        v.diffs.clear();
        for(auto& z:db_to_save_Z.cells)
        {
            if(z.first.size()==28)
                v.diffs[z.first]=z.second;
        }

        REF_getter<MsgData::BlockInfo> block = new MsgData::BlockInfo();
        // block->prev_root_hash = prev_root_hash_Z;
        block->new_root_hash1 = new_root_hash;

        block->attachment_hash = t.att_data->getHash();
        block->heart_beat = r->heart_beat;
        Blake2bHasher h;
        for(auto & z:r->transaction_bodies)
        {
            z->update(h);
        }
        block->tx_hash_Z.container=h.final();

        Blake2bHasher hh;
        for(auto &z: v.diffs)
        {
            hh.update(z.first);
            hh.update(z.second);
        }
        block->diff_hash.container=hh.final();

        REF_getter<MsgData::ValidateBlockRSP> rsp = new MsgData::ValidateBlockRSP();
        rsp->node_validator = this_node_name;
        rsp->blockInfo = block;
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
