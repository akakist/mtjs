#include "commonError.h"
#include "ioBuffer.h"
#include "blake2bHasher.h"
// #include "bigint.h"
#include "REF.h"
#include "mutexInspector.h"
#include "nodeService.h"
// #include "md/md_DelayNotificationREQ.h"
#include "md/md_BlockAcceptedREQ.h"
#include "QUORUM.h"
#include "blst_cp.h"
#include "NODE_id.h"
#include "route_t.h"
#include <cstddef>
#include <vector>
inline uint64_t read_uint64(const uint8_t* data) {
    return (static_cast<uint64_t>(data[0]) << 56) |
           (static_cast<uint64_t>(data[1]) << 48) |
           (static_cast<uint64_t>(data[2]) << 40) |
           (static_cast<uint64_t>(data[3]) << 32) |
           (static_cast<uint64_t>(data[4]) << 24) |
           (static_cast<uint64_t>(data[5]) << 16) |
           (static_cast<uint64_t>(data[6]) << 8)  |
           (static_cast<uint64_t>(data[7]));
}

REF_getter<hb_nodes_state> Node::Service::build_node_lists(const THASH_id& prev_block_hash, time_t block_timestamp)
{
    // auto nl=db->getNodeListNoCreate();
    // auto l=nl->getList();
    // logNode("build_node_lists");
    REF_getter<hb_nodes_state> ret=new hb_nodes_state;

    // auto& cli=cli_leader_info[h->prev_root_hash_1][h->block_timestamp];

    // cli.allnodes.clear();
    // cli.position_in_allodes.clear();
    auto ll=db_state->getAllNodes();
    std::map<uint64_t, std::map<NODE_id, REF_getter<bc_node>>> result;
    // std::vector<NodeElement> allnodes;
    // std::map<NODE_id,size_t> position_in_allodes;

    for(auto &x: ll)
    {
        // x->
        std::string seed=x->getName().container+prev_block_hash.container+std::to_string(block_timestamp);
        auto h=blake2b_hash(seed);
        if(h.container.size()!=32) throw CommonError("if(h.container.size()!=32)");
        auto w=read_uint64((uint8_t*)h.container.data());
        auto fs=x->get_full_stake();
        if(fs)
            w/=x->get_full_stake();
        result[w].insert_or_assign(x->getName(),x);
    }
    for(auto& x:result)
    {
        for(auto& z:x.second)
        {
            ret->position_in_allodes[z.second->getName()]=ret->allnodes.size();
            ret->allnodes.push_back(z.second->getElement());
        }
    }
    // std::string bn;
    // for(int i=0;i<ret->allnodes.size();i++)
    // {
    //     bn+="NORDER "+std::to_string(i)+" "+ret->allnodes[i].name.container+"; ";
    //     // logNode("BN %d %s",i,cli.allnodes[i].name.container.c_str());
    // }
    // logNode("bn %s",bn.c_str());
    // for(auto& z: ret->position_in_allodes)
    // {
    //     // logNode("PIAN %s %d",z.first.container.c_str(),z.second);

    // }
    return ret;
}

bool Node::Service::HeartBeatRSP(const MsgData::HeartBeatRSP *m, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    stage_is_working=iUtils->getNow();
    auto prev_root_hash=prev_root_hash_Z();
    auto &li = l_blocks[prev_root_hash].leader_info;
    if (prev_root_hash != m->payload_heart_beat->prev_root_hash_1)
    {
        logNode("heat beat expired %s %s", prev_root_hash.str().c_str(), m->payload_heart_beat->prev_root_hash_1.str().c_str());
        return false;
    }
    // logNode("HeartBeatRSP from node %s", src_node.container.c_str());
    auto n = db_state->getNodeNoCreate(m->node_signer);
    if (!n.valid())
    {
        logNode("if(!n.valid())");
        return false;
    }
    {
        li.HeartBeatRSP_m.insert_or_assign(m->node_signer, m);
    }
    // auto mf=getMetaFull();

    uint64_t hb_staked = 0;
    uint64_t full_stake = 0;
    if (iUtils->getNow() > li.confirm_leader_sent + _1sec)
    {
        bool matched = true;
        if (li.HeartBeatRSP_m.empty())
            throw CommonError("if(li.responses.empty())");

        {
            for (auto &z : li.HeartBeatRSP_m)
            {
                auto n=db_state->getNodeNoCreate(z.second->node_signer);
                auto stake=n->get_full_stake();
                hb_staked+=stake;
                // auto nn = root->getNode(z.second->node_signer,db_state.get());
                // hb_staked += nn->get_full_stake();
            }
        }
    }
    auto & cli=cli_leader_info[prev_root_hash_Z()][m->payload_heart_beat->block_timestamp];
    // auto hbstate=cli.nodes_hb_state;
    if(!cli.nodes_hb_state.valid())
    throw CommonError("if(!cli.nodes_hb_state.valid())");
    for(auto& z: cli.nodes_hb_state->allnodes)
    {
        full_stake+=z.stake_A;
    }
    // logNode("hb resp stake %lld total stake %lld (%ld)",hb_staked,full_stake,m->payload_heart_beat->block_timestamp);
    auto pers = (hb_staked * 100) / full_stake;

    if (pers > QUORUM && (iUtils->getNow() > li.confirm_leader_sent+ _1sec))
    {
        logNode("HB quorum OK");
        li.confirm_leader_sent = iUtils->getNow();
        {

            REF_getter<MsgData::ConfirmLeaderREQ> rt = new MsgData::ConfirmLeaderREQ();
            rt->hb = m->payload_heart_beat;
            // auto& cli=cli_leader_info[prev_root_hash_Z()][];

                broadcast_MsgEvent(rt.get(), cli.nodes_hb_state->allnodes);
        }
    }
    XPASS;
    return true;
}
void Node::Service::reply_HeartBeatRSP(const MsgData::HeartBeatREQ *h, const route_t &route)
{
    stage_is_working=iUtils->getNow();
    REF_getter<MsgData::HeartBeatRSP> hbr = new MsgData::HeartBeatRSP();
    hbr->payload_heart_beat = h;
    hbr->node_signer = this_node_name;

    pass_NodeMsgRSP(hbr.get(),route);

}

bool Node::Service::HeartBeatREQ(const MsgData::HeartBeatREQ *h,const MsgData::BlockAcceptedREQ *remote_prev_lc, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    
    stage_is_working=iUtils->getNow();

    // logNode("HeartBeatREQ from %s",src_node.container.c_str());
    if(!db_state->sync_empty)
    {
        MUTEX_INSPECTOR;
        logNode("HeartBeatREQ if(!db_state->sync_empty)");
        return true;
    }
    auto ct=time(NULL);
    if(h->block_timestamp<ct-2 || h->block_timestamp>ct+2)
    {
        MUTEX_INSPECTOR;
        logNode("hb block_timestamp invalid %ld %ld",h->block_timestamp,ct);
        return true;
    }

    auto local_prev_block=prev_block;
    bool remote_verified=false;
    bool local_verified=false;
    remote_verified=verify_block(remote_prev_lc);
    local_verified=verify_block(local_prev_block);

    if(local_verified && !remote_verified)
    {
        MUTEX_INSPECTOR;
        auto& cli=cli_leader_info[h->prev_root_hash_1][h->block_timestamp];
        if(iUtils->getNow() - cli.heart_beat_sent > _1sec * HEART_BEAT_SENT_TIMEOUT && !cli.get_node_leader().valid())
        {
            // logNode("if(iUtils->getNow() - cli.heart_beat_sent > _1sec * HEART_BEAT_SENT_TIMEOUT)");
            logNode("do_heart_beat(); 11");
            auto hb=do_heart_beat(h->block_timestamp);
            cli.heart_beat_sent=iUtils->getNow();
            return true;
        }
        return true;
    }
    /// если локально нет ластблока, то делаем догон.
    if(remote_verified && !local_verified)
    {
        MUTEX_INSPECTOR;
        /// если локально нет сертиката, нода стартанула с генезиса, а у удаленной есть сертификат
        /// то надо синхронизироваться, переходим в синк, не отвечаем
        logNode("if(remote_verified && !local_verified) do sync return");
        if(db_state->sync_empty){
            // state_Z = STATE_SYNCING;
            logNode("start SYNC");
            prev_block=remote_prev_lc;
            do_sync(src_node, remote_prev_lc->blockInfo->new_root_hash1);
        }
        return true;
    }
    /// если оба проверены, то сравниваем парамс касательно догона. 
    if(remote_verified && local_verified)
    {
        MUTEX_INSPECTOR;
    
        if(remote_prev_lc->blockInfo->heart_beat->new_epoch < local_prev_block->blockInfo->heart_beat->new_epoch)
        {
            /// ничего не делаем, пусть догоняет иначе,
        MUTEX_INSPECTOR;
            logNode("if(remote_prev_lc->heart_beat->new_epoch (%s) < local_lc->heart_beat->new_epoch) return",src_node.container.c_str());

            return true;
        }
        else if(remote_prev_lc->blockInfo->heart_beat->new_epoch > local_prev_block->blockInfo->heart_beat->new_epoch)
        {
            /// эпоха меньше, чем у ремоте, делаем догон
            MUTEX_INSPECTOR;
            // state_Z = STATE_SYNCING;
            logNode("START SYNCING");
            prev_block=remote_prev_lc;
            do_sync(src_node,remote_prev_lc->blockInfo->new_root_hash1);
            return true;
        }
        else if(remote_prev_lc->blockInfo->heart_beat->new_epoch == local_prev_block->blockInfo->heart_beat->new_epoch)
        {
        MUTEX_INSPECTOR;
    
                /// оба в одинаковой эпохе
                
                /// просто проверка на всякий случай на сплит брейн.
                if(remote_prev_lc->blockInfo->heart_beat->prev_root_hash_1!=local_prev_block->blockInfo->heart_beat->prev_root_hash_1)
                {
        MUTEX_INSPECTOR;
                        logNode(R"( --------------- SPLIT BRAIN DETECTED
if(remote_prev_lc->blockInfo->heart_beat->prev_root_hash!=local_lc->blockInfo->heart_beat->prev_root_hash)
remote_prev_lc->blockInfo->heart_beat->prev_root_hash %s local_lc->blockInfo->heart_beat->prev_root_hash %s)", 
                            remote_prev_lc->blockInfo->heart_beat->prev_root_hash_1.str().c_str(), local_prev_block->blockInfo->heart_beat->prev_root_hash_1.str().c_str());
                        return true;
                }
                
                if(prev_root_hash_Z()!=h->prev_root_hash_1)    
                {
        MUTEX_INSPECTOR;
    
                    /// нужно сделать голосование за прев-блок
                    /// делаем просто хартбит. В это случае поврежденная нода отваливается 
                    // по missed_rounds. Вывезет та нода, у которой консенсусный рут хеш
                    /// нужно искать гонки, где-то с мутексами проблема есть.
                    /// с другой стороны такой вариант, 
                    /// когда нода отваливается из-за очень редкой ошибки, уже пойдет.
                    /// главное - нет затыкания протокола
                    logNode(R"( --------------- SPLIT BRAIN 2 DETECTED
if(prev_root_hash_Z!=h->prev_root_hash)
        prev_root_hash_Z %s 
        h->prev_root_hash %s
        local_lc->heart_beat->prev_root_hash_1 %s
        remote_prev_lc->heart_beat->new_epoch.container %ld
        local_lc->heart_beat->new_epoch.container %ld
        from node %s )", 
            prev_root_hash_Z().str().c_str(), 
            h->prev_root_hash_1.str().c_str(),
            local_prev_block->blockInfo->heart_beat->prev_root_hash_1.str().c_str(),
            remote_prev_lc->blockInfo->heart_beat->new_epoch,
            local_prev_block->blockInfo->heart_beat->new_epoch,
            src_node.container.c_str());
                    return true;
                }
                
     
        }
    }

    bool need_reply = false;
    bool need_do_heart_beat = false;
    auto& cli=cli_leader_info[h->prev_root_hash_1][h->block_timestamp];
    auto nl=cli.get_node_leader();
    // REF_getter<hb_nodes_state> ns=NULL;

    if(!nl.valid())
    {
        MUTEX_INSPECTOR;
        cli.nodes_hb_state=build_node_lists(h->prev_root_hash_1,h->block_timestamp);
    }
    // if(!nl.valid() && )
    
    if(nl.valid())
    {
        MUTEX_INSPECTOR;
        /// ignore if blocktimestamp changed
        if(nl->block_timestamp!=h->block_timestamp)
        {
            logNode("ignore if blocktimestamp changed");
            return true;
        }
    }
    if(!nl.valid())
    {
        MUTEX_INSPECTOR;
        if(isNodeGreater(cli,this_node_name, h->node_leader) )
        {
        MUTEX_INSPECTOR;
            auto hb=do_heart_beat(h->block_timestamp);
            // nl->new node_leader=this_node_name;
            cli.heart_beat_sent=iUtils->getNow();
            return true;
        }
        else
        {
        MUTEX_INSPECTOR;
            cli.set_node_leader(h);
            cli.nodes_hb_state=build_node_lists(h->prev_root_hash_1,h->block_timestamp);
            reply_HeartBeatRSP(h,route);
            return true;
        }
    }
    if(nl.valid())
    {
        MUTEX_INSPECTOR;
        if(isNodeGreater(cli,this_node_name, h->node_leader) && isNodeGreater(cli,this_node_name, nl->node_leader))
        {
        MUTEX_INSPECTOR;
                auto hb=do_heart_beat(h->block_timestamp);
                nl->node_leader=this_node_name;
                cli.heart_beat_sent=iUtils->getNow();
                return true;
        }
        if(isNodeGreater(cli,h->node_leader, nl->node_leader))
        {
        MUTEX_INSPECTOR;
            reply_HeartBeatRSP(h,route);
            nl->node_leader=h->node_leader;
            cli.heart_beat_sent=iUtils->getNow();
            return true;
        }

    }
    
    
    return true;
}
bool Node::Service::ConfirmLeaderREQ(const MsgData::ConfirmLeaderREQ *h, const NODE_id &src_node, const route_t &route)

{

    MUTEX_INSPECTOR;
    // logNode("ConfirmLeaderREQ from %s",src_node.container.c_str());
    if(!db_state->sync_empty)
    {
        logNode("if(!db_state->sync_empty)");
        return true;
    }
    stage_is_working=iUtils->getNow();


    bool need_replace = false;

    if (prev_root_hash_Z() != h->hb->prev_root_hash_1)
    {
            logNode("ConfirmLeaderREQ: invalid root hash, no answer this node %s src_node %s '%s' '%s'", this_node_name.container.c_str(), src_node.container.c_str(), 
            prev_root_hash_Z().str().c_str(),h->hb->prev_root_hash_1.str().c_str());
        return true;
    }
    if(!h->hb.valid())
        throw CommonError("if(!h->hb.valid()) %s %d",__FILE__,__LINE__);

        // bool need_reply = false;
    auto &cli=cli_leader_info[h->hb->prev_root_hash_1][h->hb->block_timestamp];
    auto nl=cli.get_node_leader();
    if (!nl.valid())
    {
        throw CommonError("!!!!!!!!!!!!!!!!!!! if (!nl.valid()) %s %d",__FILE__,__LINE__);
        // if(nl->node_leader!=src_node)
        // {
        //     logNode("if(cli.get_node_leader()!=src_node)");
        // }
        // cli.set_node_leader(h->hb);
        // build_node_lists(h->hb);
    }
    if (!h->hb->equals(nl))
    {
        logNode("if (!h->hb->equals(nl))  %s %s",nl->node_leader.container.c_str(),h->hb->node_leader.container.c_str());
        logNode("ConfirmLeaderREQ failed - local node %s,  req node %s",nl->node_leader.container.c_str(),h->hb->node_leader.container.c_str());
        return true;
    }
    else
    {
        // logNode("ConfirmLeaderRSP %s",h->hb->node_leader.container.c_str());
        REF_getter<MsgData::ConfirmLeaderRSP> hbr = new MsgData::ConfirmLeaderRSP();
        hbr->hb = h->hb;
        hbr->node_signer = this_node_name;
        // hbr->sig.sign(my_sk_bls, blake2b_hash(h->hb->getBuffer()).container);

        pass_NodeMsgRSP(hbr.get(),route);
        cli.confirm_leader_sent=iUtils->getNow();
    }
    return true;
}

bool Node::Service::ConfirmLeaderRSP(const MsgData::ConfirmLeaderRSP *m, const NODE_id &src_node, const route_t &route)
{
    XTRY;
        // logNode("@@ ConfirmLeaderRSP from %s",src_node.container.c_str());

    if(!db_state->sync_empty)
    {
        return true;
    }
    stage_is_working=iUtils->getNow();

    auto prev_root_hash=prev_root_hash_Z();
    auto &li = l_blocks[prev_root_hash].leader_info;
    // auto &li = hbs.leader_info;
    if (prev_root_hash != m->hb->prev_root_hash_1)
    {
        logNode("heat beat expired %s %s", prev_root_hash.str().c_str(), m->hb->prev_root_hash_1.str().c_str());
        return false;
    }

    {
        li.ConfirmLeaderRSP_m.insert_or_assign(m->node_signer, m);
    }
    // auto mf=getMetaFull();
    uint64_t fullstake=0;
    auto &cli=cli_leader_info[prev_root_hash_Z()][m->hb->block_timestamp];
    if(!cli.nodes_hb_state.valid())
        throw CommonError("if(!cli.nodes_hb_state.valid())");

    for(auto&z :cli.nodes_hb_state->allnodes)
    {
        fullstake+=z.stake_A;
    }
    uint64_t hb_staked = 0;
    {
        bool matched = true;
        if (li.ConfirmLeaderRSP_m.empty())
            throw CommonError("if(li.responses.empty())");

        for (auto &z : li.ConfirmLeaderRSP_m)
        {
            auto n=db_state->getNodeNoCreate(z.second->node_signer);
            auto stake = n->get_full_stake();
            hb_staked += stake;
        }
    }
    auto pers = hb_staked * 100 / fullstake;
    // logNode("ConfirmLeaderRSP pers %ld",pers);

    if (pers > QUORUM)
    {
        // make_leader_certificate();
        if (!li.request_for_transactions_sent)
        {
            logNode("lEAder approved %s", m->hb->node_leader.container.c_str());
            li.request_for_transactions_sent = true;
            // li.leader_cert_2 = lc;
    // logNode("ConfirmLeaderRSP do_request_for_transactions");
            do_request_for_transactions(li);
        }
    }
    XPASS;
    return true;
}

REF_getter<MsgData::HeartBeatREQ> Node::Service::do_heart_beat(time_t hbtime)
{
    MUTEX_INSPECTOR;
    stage_is_working=iUtils->getNow();
    l_blocks.clear();
    c_blocks.clear();
    // auto mf=getMetaFull();
    // auto prev=prev_block;
    REF_getter<MsgData::HeartBeatREQ> hb_req =
        new MsgData::HeartBeatREQ(prev_root_hash_Z(),
                                    epoch_current(),
                                    this_node_name,  hbtime);

    // auto prev_lc=prev_block;
    REF_getter<MsgData::LcEnvelopeREQ> lce =new MsgData::LcEnvelopeREQ(hb_req->getBuffer(),prev_block.valid()?prev_block->getBuffer():"");
    // logNode("broadcast heart beat");
    l_blocks[prev_root_hash_Z()].leader_info.leader_cert_2=hb_req;
    auto &cli=cli_leader_info[prev_root_hash_Z()][hb_req->block_timestamp];
    if(!cli.nodes_hb_state.valid())
    {
        cli.nodes_hb_state=build_node_lists(hb_req->prev_root_hash_1,hb_req->block_timestamp);
    }
    if(!cli.nodes_hb_state.valid())
        throw CommonError("if(!cli.nodes_hb_state.valid())");
    std::string from;
    if(cli.get_node_leader().valid())
    {
        from=cli.get_node_leader()->node_leader.container;
    }
    // logNode("set node leader from %s to %s",from.c_str(),hb_req->node_leader.container.c_str());
    cli.set_node_leader(hb_req);
    auto hn=build_node_lists(hb_req->prev_root_hash_1,hb_req->block_timestamp);
    cli.nodes_hb_state=hn;
    cli.heart_beat_sent=iUtils->getNow();
    // auto mm=mf->full_broadcast;
    // std::string s;
    // for(auto & z:mm)
    // {
    //     s+=" "+z.container;
    // }
    // logErr2("validators %s",s.c_str());
    broadcast_MsgEvent(lce.get(),cli.nodes_hb_state->allnodes);

    return hb_req;
}

