#include "commonError.h"
#include "ioBuffer.h"
#include "blake2bHasher.h"
// #include "bigint.h"
#include "REF.h"
#include "mutexInspector.h"
#include "nodeService.h"
#include "md/md_DelayNotificationREQ.h"
#include "md/md_BlockAcceptedREQ.h"
#include "QUORUM.h"
#include "blst_cp.h"
#include "NODE_id.h"
#include "route_t.h"
#include <cstddef>
#include <vector>
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

    auto n = db_state->getNodeNoCreate(m->node_signer);
    if (!n.valid())
    {
        logNode("if(!n.valid())");
        return false;
    }
    {
        li.HeartBeatRSP_m.insert_or_assign(m->node_signer, m);
    }
    auto mf=getMetaFull(m->payload_heart_beat->block_timestamp);

    uint64_t hb_staked = 0;
    if (iUtils->getNow() > li.confirm_leader_sent + _1sec)
    {
        bool matched = true;
        if (li.HeartBeatRSP_m.empty())
            throw CommonError("if(li.responses.empty())");

        {
            for (auto &z : li.HeartBeatRSP_m)
            {
                auto stake=mf->getStake(z.second->node_signer);
                hb_staked+=stake;
            }
        }
    }
    auto pers = (hb_staked * 100) / mf->total_full_stake;

    if (pers > QUORUM && (iUtils->getNow() > li.confirm_leader_sent+ _1sec))
    {
        logNode("HB quorum OK");
        li.confirm_leader_sent = iUtils->getNow();
        {

            REF_getter<MsgData::ConfirmLeaderREQ> rt = new MsgData::ConfirmLeaderREQ();
            rt->hb = m->payload_heart_beat;

                broadcast_MsgEvent_via_broadcaster(rt.get(),mf);
        }
    }
    XPASS;
    return true;
}
void Node::Service::reply_HeartBeatRSP(const MsgData::HeartBeatREQ *h, const route_t &route)
{
    logNode("@@ %s",__func__);
    stage_is_working=iUtils->getNow();
        

    REF_getter<MsgData::HeartBeatRSP> hbr = new MsgData::HeartBeatRSP();
    hbr->payload_heart_beat = h;
    hbr->node_signer = this_node_name;

    pass_NodeMsgRSP(hbr.get(),route);

}
bool Node::Service::HeartBeatREQ(const MsgData::HeartBeatREQ *h,const MsgData::BlockAcceptedREQ *remote_prev_lc, const NODE_id &src_node, const route_t &route, bool * need_continue_broadcast)
{
        logNode("@@ %s from %s",__func__,h->node_leader.container.c_str());

    MUTEX_INSPECTOR;
    if(!need_continue_broadcast)
    {
        logNode("if(!need_continue_broadcast)");
        return true;
    }
    *need_continue_broadcast=true;
    
    stage_is_working=iUtils->getNow();

    if(h->node_leader==this_node_name)
    {
        logNode("reply_HeartBeatRSP if(h->node_leader==this_node_name)");
            reply_HeartBeatRSP(h,route);
            *need_continue_broadcast=true;
            return true;

    }
        
    auto tw_local=time(NULL) / HB_TIME_WINDOW;
    auto tw_remote=h->block_timestamp / HB_TIME_WINDOW;

    if(tw_remote < tw_local-1 || tw_remote > tw_local+1)
    {
        logNode("if(tw_remote < tw_local-1 || tw_remote > tw_local+1)");
        return true;
    }
    if(tw_remote < tw_local)
    {
        logNode("if(tw_remote < tw_local)");
    }
    
    // logNode("HeartBeatREQ");
    if(!db_state->sync_empty)
    {
        logNode("HeartBeatREQ if(!db_state->sync_empty)");
        return true;
    }

    auto& cli=cli_leader_info[h->prev_root_hash_1];
    if(cli.node_leader.valid())
    {
        if(cli.node_leader->block_timestamp / HB_TIME_WINDOW < h->block_timestamp / HB_TIME_WINDOW)
        {
            logNode("cli.node_leader=NULL; t_WIN not match");
            cli.node_leader=NULL;
        }
    }

    // bool need_reply = false;
    auto local_prev_block=prev_block;
    
    bool remote_verified=false;
    bool local_verified=false;
    remote_verified=verify_block(remote_prev_lc);
    local_verified=verify_block(local_prev_block);
    

    if(local_verified && !remote_verified)
    {
        /// не отвечаем, поскольку ремоте нода не имеет сертификата
        logNode("if(local_verified && !remote_verified) return send DelayNotificationREQ");
        REF_getter<MsgData::DelayNotificationREQ> d=new MsgData::DelayNotificationREQ;
        d->lc=local_prev_block;
        auto buffer = d->getBuffer();
        auto n=db_state->getNodeNoCreate(src_node);
        if(!n.valid())
            throw CommonError("if(!n.valid())");
        sendEvent(n->get_ip(), ServiceEnum::Node,
                new bcEvent::NodeMsgREQ(this_node_name, node_start_timestamp, seqId2++, sign_ed(my_sk_ed, blake2b_hash(buffer).container), buffer, ListenerBase::serviceId));
        *need_continue_broadcast=false;
        return true;
    }
    

    if(!remote_verified && !local_verified)    /// block 0
    {
        /// отвечаем, поскольку это кейс старта с генезиса
        logNode("if(!remote_verified && !local_verified) ");
        auto mf=getMetaFull(h->block_timestamp);
        if(isNodeGreater(this_node_name,h->node_leader,mf))
        {
            if(iUtils->getNow()-cli.heart_beat_sent[h->block_timestamp / HB_TIME_WINDOW] > _1sec * HEART_BEAT_SENT_TIMEOUT)
            {
                logNode("if(isNodeGreater(this_node_name,h->node_leader,mf))");
                auto hb=do_heart_beat();
                // cli.node_leader=hb;
                // cli.heart_beat_sent[h->block_timestamp/HB_TIME_WINDOW]=iUtils->getNow();
                *need_continue_broadcast=false;
                return true;
            }
        }
        else
        {
            logNode("reply_HeartBeatRSP(h,route);");
            reply_HeartBeatRSP(h,route);
            *need_continue_broadcast=true;
            return true;
        }
    }
    
    if(remote_verified && !local_verified)
    {
        /// если локально нет сертиката, нода стартанула с генезиса, а у удаленной есть сертификат
        /// то надо синхронизироваться, переходим в синк, не отвечаем
        logNode("if(remote_verified && !local_verified) do sync return");
        *need_continue_broadcast=true;
        if(db_state->sync_empty){
            // state_Z = STATE_SYNCING;
            logNode("start SYNC");
            prev_block=remote_prev_lc;
            do_sync(src_node, remote_prev_lc->blockInfo->new_root_hash1);
        }
        return true;
    }
    
    if(remote_verified && local_verified)
    {
        if(remote_prev_lc->blockInfo->heart_beat->new_epoch < local_prev_block->blockInfo->heart_beat->new_epoch)
        {
        /// проверка на эпоху, если эпоха у ремоте меньше, то командуем ей обновиться.
            logNode("if(remote_prev_lc->heart_beat->new_epoch (%s) < local_lc->heart_beat->new_epoch) return",src_node.container.c_str());
            REF_getter<MsgData::DelayNotificationREQ> d=new MsgData::DelayNotificationREQ;
            d->lc=local_prev_block;
            auto buffer = d->getBuffer();
            auto n=db_state->getNodeNoCreate(src_node);
            if(!n.valid())
                throw CommonError("if(!n.valid())");

            sendEvent(n->get_ip(), ServiceEnum::Node,
                    new bcEvent::NodeMsgREQ(this_node_name, node_start_timestamp, seqId2++, sign_ed(my_sk_ed, blake2b_hash(buffer).container), buffer, ListenerBase::serviceId));
            *need_continue_broadcast=false;
            return true;
        }
        else if(remote_prev_lc->blockInfo->heart_beat->new_epoch > local_prev_block->blockInfo->heart_beat->new_epoch)
        {
            /// если наша эпоха меньше , то сами делаем догон
            MUTEX_INSPECTOR;
            // state_Z = STATE_SYNCING;
            logNode("START SYNCING");
            prev_block=remote_prev_lc;
            do_sync(src_node,remote_prev_lc->blockInfo->new_root_hash1);
            *need_continue_broadcast=true;
            return true;
        }
        else if(remote_prev_lc->blockInfo->heart_beat->new_epoch == local_prev_block->blockInfo->heart_beat->new_epoch)
        {
    
                /// оба в одинаковой эпохе
                
                /// просто проверка на всякий случай.
                if(remote_prev_lc->blockInfo->heart_beat->prev_root_hash_1!=local_prev_block->blockInfo->heart_beat->prev_root_hash_1)
                {
                    /// сплит брейн. в сохраненных блоках
                    logNode(R"( --------------- SPLIT BRAIN DETECTED
if(remote_prev_lc->blockInfo->heart_beat->prev_root_hash!=local_lc->blockInfo->heart_beat->prev_root_hash)
remote_prev_lc->blockInfo->heart_beat->prev_root_hash %s local_lc->blockInfo->heart_beat->prev_root_hash %s)", 
                            remote_prev_lc->blockInfo->heart_beat->prev_root_hash_1.str().c_str(), local_prev_block->blockInfo->heart_beat->prev_root_hash_1.str().c_str());
                    *need_continue_broadcast=true;
                    return true;
                }
                
                if(prev_root_hash_Z()!=h->prev_root_hash_1)    
                {
                    /// сплит брейн для текущего блока.
    
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

            *need_continue_broadcast=true;

                    return true;
                }

                // bool need_do_heart_beat=false;
                // bool need_reply = false;
                
                if(cli.node_leader.valid())
                {
                    logNode("if(cli.node_leader.valid())");
                    logNode("cli.node_leader->block_timestamp/HB_TIME_WINDOW == h->block_timestamp/HB_TIME_WINDOW %d %d",cli.node_leader->block_timestamp/HB_TIME_WINDOW,h->block_timestamp/HB_TIME_WINDOW);
                    if(cli.node_leader->block_timestamp/HB_TIME_WINDOW == h->block_timestamp/HB_TIME_WINDOW)
                    {
                        
                        auto mf=getMetaFull(h->block_timestamp);
                        if(isNodeGreater(h->node_leader, cli.node_leader->node_leader, mf) ) 
                        {
                            logNode("h->node_leader > cli.node_leader->node_leader");
                            cli.node_leader=h;
                            reply_HeartBeatRSP(h,route);
                            logNode("reply_HeartBeatRSP");
                            *need_continue_broadcast=true;
                            return true;
                        }
                        else 
                        {
                            logNode("h->node_leader < cli.node_leader->node_leader");
                                // if(iUtils->getNow()-cli.heart_beat_sent[h->block_timestamp / HB_TIME_WINDOW] > _1sec * HEART_BEAT_SENT_TIMEOUT)
                                // {
                                //     auto hb=do_heart_beat();
            
                                // }
                                *need_continue_broadcast=false;
                        }

                    }
                    else
                    {
                        logNode("timewin not matched");
                    }

                }
                else /// !cli.node_leader.valid()
                {
                    auto mf=getMetaFull(h->block_timestamp);
                   if(isNodeGreater(this_node_name, h->node_leader, mf) ) 
                   {
                        if(iUtils->getNow()-cli.heart_beat_sent[h->block_timestamp / HB_TIME_WINDOW] > _1sec * HEART_BEAT_SENT_TIMEOUT)
                        {
                            do_heart_beat();
                        }
                        return false;
                        // need_do_heart_beat = true;
                        // *need_continue_broadcast=false;
                   }
                   else
                   {
                        cli.node_leader=h;
                        reply_HeartBeatRSP(h,route);
                        *need_continue_broadcast=true;
                        return true;

                   }

                }
     
                // if(iUtils->getNow()-cli.heart_beat_sent[h->block_timestamp / HB_TIME_WINDOW] > _1sec * HEART_BEAT_SENT_TIMEOUT)
                // {
                //     logNode("if(iUtils->getNow()-cli.heart_beat_sent > _1sec * HEART_BEAT_SENT_TIMEOUT)");
                    
                //     if(isNodeGreater(this_node_name, h->node_leader, mf) 
                //         || (cli.node_leader.valid() && isNodeGreater(this_node_name, cli.node_leader->node_leader, mf)))
                //     {
                //         logNode("@@ %s > %s",this_node_name.container.c_str(), h->node_leader.container.c_str());
                //         // ci.node_leader=new MsgData::HeartBeatREQ(prev_root_hash_Z,);
                //         auto hb=do_heart_beat(mf);
                //         logNode("setup node leader do heart beat");
                //         // cli.node_leader=hb;
                //         // cli.heart_beat_sent[h->block_timestamp / HB_TIME_WINDOW]=iUtils->getNow();
                //         *need_continue_broadcast=false;
                //         return true;
                //     }
                //     else 
                //     {
                //         logNode("@@ %s <= %s",this_node_name.container.c_str(), h->node_leader.container.c_str());

                //     }
                // }
                // if(!cli.node_leader.valid())
                // {
                //     logNode("setup NL if(!cli.node_leader.valid())");
                //     cli.node_leader=h;
                // }
                // if(h->block_timestamp/HB_TIME_WINDOW > cli.node_leader->block_timestamp / HB_TIME_WINDOW)
                // {
                //     logNode("setup NL if(h->block_timestamp/HB_TIME_WINDOW > cli.node_leader->block_timestamp / HB_TIME_WINDOW)");
                //     cli.node_leader=h;
                // }
                // // if(ci.node_leader.container.empty())
                //     // ci.node_leader=this_node_name;
                // auto mf=getMetaFull(h->block_timestamp);
                // if (cli.node_leader->node_leader.container.empty() || isNodeGreater(h->node_leader, cli.node_leader->node_leader,mf))
                // {
                //     logNode("setup NL if (cli.node_leader->node_leader.container.empty() || isNodeGreater(h->node_leader, cli.node_leader->node_leader,mf)) and make reply_HeartBeatRSP");
                //     cli.node_leader=h;
                //     reply_HeartBeatRSP(h,route);
                //     *need_continue_broadcast=true;
                //     return true;
                // }
        }
    }
    return true;
}
bool Node::Service::ConfirmLeaderREQ(const MsgData::ConfirmLeaderREQ *h, const NODE_id &src_node, const route_t &route)

{
    MUTEX_INSPECTOR;
    if(!db_state->sync_empty)
    {
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
    bool need_reply = false;
    auto &cli=cli_leader_info[h->hb->prev_root_hash_1];
    if (!cli.node_leader.valid())
        cli.node_leader = h->hb;
    if (!h->hb->equals(cli.node_leader))
    {
        return true;
    }
    else
        need_reply = true;

    if (need_reply)
    {
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
    auto mf=getMetaFull(m->hb->block_timestamp);
    uint64_t hb_staked = 0;
    {
        bool matched = true;
        if (li.ConfirmLeaderRSP_m.empty())
            throw CommonError("if(li.responses.empty())");

        for (auto &z : li.ConfirmLeaderRSP_m)
        {
            auto stake = mf->getStake(z.second->node_signer);
            hb_staked += stake;
        }
    }
    auto pers = hb_staked * 100 / mf->total_full_stake;

    if (pers > QUORUM)
    {
        // make_leader_certificate();
        if (!li.request_for_transactions_sent)
        {
            logNode("lEAder approved %s", m->hb->node_leader.container.c_str());
            li.request_for_transactions_sent = true;
            // li.leader_cert_2 = lc;
    // logNode("ConfirmLeaderRSP do_request_for_transactions");
            do_request_for_transactions(li,mf);
        }
    }
    XPASS;
    return true;
}
bool Node::Service::LcEnvelopeREQ(const MsgData::LcEnvelopeREQ* m, const NODE_id & src_node, const route_t& route, bool *need_continue_broadcast)
{
    MUTEX_INSPECTOR;
   
    inBuffer in(m->msg);
    auto id = in.get_PN();
    REF_getter<MsgData::Base> msg = msgFactory.create(id);
    msg->unpack(in);
    
    REF_getter<MsgData::BlockAcceptedREQ> lc;
    if(m->prev_lc.size())
    {
        lc=new MsgData::BlockAcceptedREQ;
        inBuffer in2(m->prev_lc);
        lc->unpack2(in2);

    }
 
    switch (msg->type)
    {
    case msgid::HeartBeatREQ:
        return HeartBeatREQ(static_cast<const MsgData::HeartBeatREQ *>(msg.get()),lc.valid()?lc.get():NULL, src_node, route, need_continue_broadcast);
    default:
        throw CommonError("2 MsgData %s", msgName(msg->type));
    }

    return true;
}

REF_getter<MsgData::HeartBeatREQ> Node::Service::do_heart_beat()
{
    logNode("@@ %s",__func__);
    
    stage_is_working=iUtils->getNow();
        
    time_t tnow=time(NULL);
    // logNode("@@ %s",__FUNCTION__);
    // l_blocks.clear();
    // block_meta_full.clear();
    // block_meta_validator.clear();
    // c_blocks.clear();
    auto meta=getMetaFull(tnow);

    REF_getter<MsgData::HeartBeatREQ> hb_req =
        new MsgData::HeartBeatREQ(prev_root_hash_Z(),
                                    epoch_current(),
                                    this_node_name,  tnow);

    // auto prev_lc=prev_block;
    REF_getter<MsgData::LcEnvelopeREQ> lce =new MsgData::LcEnvelopeREQ(hb_req->getBuffer(),prev_block.valid()?prev_block->getBuffer():"");
    l_blocks[prev_root_hash_Z()].leader_info.leader_cert_2=hb_req;
    cli_leader_info[prev_root_hash_Z()].node_leader=hb_req;
    cli_leader_info[prev_root_hash_Z()].heart_beat_sent[tnow/HB_TIME_WINDOW]=iUtils->getNow();

    broadcast_MsgEvent_via_node(lce.get(),meta);

    return hb_req;
}

