#include "commonError.h"
#include "ioBuffer.h"
#include "blake2bHasher.h"
#include "bigint.h"
#include "REF.h"
#include "mutexInspector.h"
#include "nodeService.h"
#include "QUORUM.h"
#include "blst_cp.h"
#include "NODE_id.h"
#include "route_t.h"
#include <cstddef>
#include <vector>
bool Node::Service::HeartBeatRSP(const MsgData::HeartBeatRSP *m, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    auto &hbs = blocks_leader[prev_root_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    if (prev_root_hash_Z != m->payload_heart_beat->prev_root_hash_1)
    {
        logNode("heat beat expired %s %s", prev_root_hash_Z.str().c_str(), m->payload_heart_beat->prev_root_hash_1.str().c_str());
        return false;
    }

    auto n = root->getNode(m->node_signer);
    if (!n.valid())
    {
        logNode("if(!n.valid())");
        return false;
    }
    outBuffer o;
    m->payload_heart_beat->pack(o);
    if (!m->signature.verify(n->get_bls_pk(), blake2b_hash(o.asString()->container).container))
    {
        logNode("if(!sig_check.verify(n->bls_pk, blake2b_hash(mhbr.payload)))");
        return false;
    }
    {
        li.HeartBeatRSP_m.insert_or_assign(m->node_signer, m);
    }

    BigInt hb_staked = 0;
    if (!li.confirm_leader_sent)
    {
        blst_cpp::AggregateSignature sig_agg;
        std::vector<blst_cpp::PublicKey> pk_agg;
        bool matched = true;
        if (li.HeartBeatRSP_m.empty())
            throw CommonError("if(li.responses.empty())");

        {
            for (auto &z : li.HeartBeatRSP_m)
            {
                auto nn = root->getNode(z.second->node_signer);
                hb_staked += nn->get_full_stake();
                // logNode("nn->get_full_stake() %s",nn->get_full_stake().toString().c_str());
                pk_agg.push_back(nn->get_bls_pk());
                sig_agg.add(z.second->signature);
            }
        }
    }
    BigInt total_staked=0;
    
    auto nn=root->getAllNodes();
    for(auto& z: nn)
    {
        total_staked+=z->get_full_stake();
    }
    // logNode("hb_staked %lf total_staked %lf",hb_staked.toDouble(), total_staked.toDouble());
    // logNode()
    auto pers = (hb_staked.toDouble()) / total_staked.toDouble();

    if (pers > QUORUM && !li.confirm_leader_sent)
    {
        li.confirm_leader_sent = true;
        {

            REF_getter<MsgData::ConfirmLeaderREQ> rt = new MsgData::ConfirmLeaderREQ();
            rt->hb = m->payload_heart_beat;
            broadcast_MsgEvent(rt.get());
        }
    }
    XPASS;
    return true;
}
void Node::Service::reply_HeartBeatRSP(const MsgData::HeartBeatREQ *h, const route_t &route)
{
        stage_is_working=true;
        resetTimer();
        REF_getter<MsgData::HeartBeatRSP> hbr = new MsgData::HeartBeatRSP();
        // msg::heart_beat_rsp hba;
        hbr->payload_heart_beat = h;
        hbr->node_signer = this_node_name;
        hbr->signature.sign(my_sk_bls, blake2b_hash(h->getBuffer()).container);

        pass_NodeMsgRSP(hbr.get(),route);

}
bool Node::Service::HeartBeatREQ(const MsgData::HeartBeatREQ *h,const MsgData::LeaderCertificate *remote_prev_lc, const NODE_id &src_node, const route_t &route)
{
    if(state_Z==STATE_SYNCING)
    {
        return true;
    }
    MUTEX_INSPECTOR;
    bool need_reply = false;
    REF_getter<MsgData::LeaderCertificate>  local_lc;
    if(h->block_timestamp < (time(NULL)-60) || h->block_timestamp > (time(NULL)+60))
    {
        logNode("block timestamp not valid");
        return true;
    }
    auto llc=root->getEpoch()->prev_lc;
    if(llc.size())
    {
        local_lc=new MsgData::LeaderCertificate;
        inBuffer in(llc);
        local_lc->unpack2(in);
    }
    
    bool remote_verified=false;
    bool local_verified=false;
    if(!remote_prev_lc)
    {
        logNode("remote prtev lc NULL %s", src_node.container.c_str());
    }
    if(remote_prev_lc)
    {
        remote_verified=verify_leader_certificate(remote_prev_lc);
    }
    else logNode("!if(remote_lc.valid())");
    if(local_lc.valid())
    {
        local_verified=verify_leader_certificate(local_lc);
    }
    else logNode("!if(local_lc.valid())");

    if(local_verified && !remote_verified)
    {
        /// не отвечаем, поскольку ремоте нода не имеет сертификата
        logNode("if(local_verified && !remote_verified) return ");
        return true;
    }

    if(!remote_verified && !local_verified)    
    {
        /// отвечаем, поскольку это кейс старта с генезиса
        logNode("if(!remote_verified && !local_verified)   reply_HeartBeatRSP return ");
        // need_reply=true;
        reply_HeartBeatRSP(h,route);
        return true;
    }
    if(remote_verified && !local_verified)
    {
        /// если локально нет сертиката, нода стартанула с генезиса, а у удаленной есть сертификат
        /// то надо синхронизироваться, переходим в синк, не отвечаем
        logNode("if(remote_verified && !local_verified) do sync return");
        if(state_Z!=STATE_SYNCING){
            state_Z = STATE_SYNCING;
            logNode("do sync");
            do_sync(src_node);
        }
        return true;
    }
    if(remote_verified && local_verified)
    {
        if(remote_prev_lc->heart_beat->new_epoch < local_lc->heart_beat->new_epoch)
        {
            logNode("if(remote_prev_lc->heart_beat->new_epoch < local_lc->heart_beat->new_epoch) return");
            return true;
        }
        else if(remote_prev_lc->heart_beat->new_epoch > local_lc->heart_beat->new_epoch)
        {
            state_Z = STATE_SYNCING;
            do_sync(src_node);
            return true;
        }
        else if(remote_prev_lc->heart_beat->new_epoch == local_lc->heart_beat->new_epoch)
        {
                /// оба в одинаковой эпохе
                
                /// просто проверка на всякий случай.
                if(remote_prev_lc->heart_beat->prev_root_hash_1!=local_lc->heart_beat->prev_root_hash_1)
                {
                        logNode("if(remote_prev_lc->heart_beat->prev_root_hash!=local_lc->heart_beat->prev_root_hash)\nremote_prev_lc->heart_beat->prev_root_hash %s local_lc->heart_beat->prev_root_hash %s", remote_prev_lc->heart_beat->prev_root_hash_1.str().c_str(), local_lc->heart_beat->prev_root_hash_1.str().c_str());
                        return true;
                }
                    // throw CommonError("if(remote_prev_lc->heart_beat->prev_root_hash!=local_lc->heart_beat->prev_root_hash)");
                /// тоже проверка на всякий случай
                if(prev_root_hash_Z!=h->prev_root_hash_1)    
                {
                    /// нужно сделать голосование за прев-блок
                    /// делаем просто хартбит. В это случае поврежденная нода отваливается 
                    // по missed_rounds. Вывезет та нода, у которой консенсусный рут хеш
                    /// нужно искать гонки, где-то с мутексами проблема есть.
                    /// с другой стороны такой вариант, 
                    /// когда нода отваливается из-за очень редкой ошибки, уже пойдет.
                    /// главное - нет затыкания протокола
                    logNode("if(prev_root_hash_Z!=h->prev_root_hash)    prev_root_hash_Z %s h->prev_root_hash %s from node %s", prev_root_hash_Z.str().c_str(), h->prev_root_hash_1.str().c_str(),src_node.container.c_str());
                    auto& ci=cli_leader_info[prev_root_hash_Z];
                    if(!ci.heart_beat_sent)
                    {
                        ci.heart_beat_sent=true;
                        do_heart_beat();
                    }
                    return true;
                }
                    // throw CommonError("if(prev_root_hash_Z!=h->prev_root_hash)    ");
                
                auto& ci=cli_leader_info[h->prev_root_hash_1];
                if(ci.node_leader.container.empty())
                    ci.node_leader=this_node_name;

                if (isNodeGreaterOrEqual(h->node_leader, ci.node_leader))
                {
                    ci.node_leader=h->node_leader;
                    reply_HeartBeatRSP(h,route);
                    return true;
                }
                else
                {
                    if(!ci.heart_beat_sent)
                    {
                        ci.heart_beat_sent=true;
                        do_heart_beat();
                    }
                }
        }

    }
    return true;
}
bool Node::Service::ConfirmLeaderREQ(const MsgData::ConfirmLeaderREQ *h, const NODE_id &src_node, const route_t &route)

{
    MUTEX_INSPECTOR;
    if(state_Z==STATE_SYNCING)
    {
        return true;
    }

    resetTimer();

    bool need_replace = false;

    if (prev_root_hash_Z != h->hb->prev_root_hash_1)
    {
        logNode("invalid root hash, no answer this node %s src_node %s", this_node_name.container.c_str(), src_node.container.c_str());
        return true;
    }
    bool need_reply = false;
    if (cli_leader_info[h->hb->prev_root_hash_1].node_leader.container.empty())
        cli_leader_info[h->hb->prev_root_hash_1].node_leader = h->hb->node_leader;
    if (h->hb->node_leader != cli_leader_info[h->hb->prev_root_hash_1].node_leader )
    {
        if (isNodeGreaterOrEqual(h->hb->node_leader, cli_leader_info[h->hb->prev_root_hash_1].node_leader))
        {
            cli_leader_info[h->hb->prev_root_hash_1].node_leader = h->hb->node_leader;
            need_reply = true;
        }
    }
    else
        need_reply = true;

    if (need_reply)
    {
        REF_getter<MsgData::ConfirmLeaderRSP> hbr = new MsgData::ConfirmLeaderRSP();
        hbr->hb = h->hb;
        hbr->node_signer = this_node_name;
        hbr->sig.sign(my_sk_bls, blake2b_hash(h->hb->getBuffer()).container);


        pass_NodeMsgRSP(hbr.get(),route);
    }
    return true;
}

bool Node::Service::ConfirmLeaderRSP(const MsgData::ConfirmLeaderRSP *m, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    if(state_Z!=STATE_NORMAL)
        return true;

    auto &hbs = blocks_leader[prev_root_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    if (prev_root_hash_Z != m->hb->prev_root_hash_1)
    {
        logNode("heat beat expired %s %s", prev_root_hash_Z.str().c_str(), m->hb->prev_root_hash_1.str().c_str());
        return false;
    }

    auto n = root->getNode(m->node_signer);
    if (!n.valid())
    {
        logNode("if(!n.valid())");
        return false;
    }
    outBuffer o;
    m->hb->pack(o);
    if (!m->sig.verify(n->get_bls_pk(), blake2b_hash(o.asString()->container).container))
    {
        logNode("if(!sig_check.verify(n->bls_pk, blake2b_hash(mhbr.payload)))");
        return false;
    }
    {
        li.ConfirmLeaderRSP_m.insert_or_assign(m->node_signer, m);
    }
    BigInt hb_staked = 0;
    {
        blst_cpp::AggregateSignature sig_agg;
        std::vector<blst_cpp::PublicKey> pk_agg;
        bool matched = true;
        if (li.ConfirmLeaderRSP_m.empty())
            throw CommonError("if(li.responses.empty())");

        for (auto &z : li.ConfirmLeaderRSP_m)
        {
            sig_agg.add(z.second->sig);
            auto nn = root->getNode(z.second->node_signer);
            pk_agg.push_back(nn->get_bls_pk());
            hb_staked += nn->get_full_stake();
        }
    }
    BigInt total_staked;
    auto nn=root->getAllNodes();
    for(auto &n:nn)
    {
        total_staked+=n->get_full_stake();
    }
    auto pers = (hb_staked.toDouble()) / total_staked.toDouble();

    if (pers > QUORUM)
    {
        make_leader_certificate();
        if (!li.request_for_transactions_sent)
        {
            logNode("lEAder approved %s", m->hb->node_leader.container.c_str());
            li.request_for_transactions_sent = true;
            // li.leader_cert_2 = lc;
            do_request_for_transactions(li);
        }
    }
    XPASS;
    return true;
}

void Node::Service::do_heart_beat()
{
    blocks_leader.clear();
    // sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));
    {
        EPOCH_id e=root->getEpoch()->epoch;
        e.container+=1;
        REF_getter<MsgData::HeartBeatREQ> hb_req =
            new MsgData::HeartBeatREQ(prev_root_hash_Z,
                                      e,
                                      this_node_name, root->getEpoch()->prev_lc, time(NULL));

        REF_getter<MsgData::LcEnvelopeREQ> lce =new MsgData::LcEnvelopeREQ(hb_req->getBuffer(),root->getEpoch()->prev_lc);
        broadcast_MsgEvent(lce.get());
    }

    return;
}

void Node::Service::make_leader_certificate()
{
    auto &hbs = blocks_leader[prev_root_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    REF_getter<MsgData::LeaderCertificate> lc = new MsgData::LeaderCertificate();
    if (li.ConfirmLeaderRSP_m.empty())
        throw CommonError("if(li.ConfirmLeaderRSP_m.empty())");
    // return nullptr;
    auto msg = li.ConfirmLeaderRSP_m.begin()->second->hb;
    lc->heart_beat = li.ConfirmLeaderRSP_m.begin()->second->hb;
    for (auto &r : li.ConfirmLeaderRSP_m)
    {
        lc->agg_sig.add(r.second->sig);
        auto nn = root->getNode(r.second->node_signer);
        lc->nodes.push_back(r.second->node_signer);
    }

    li.leader_cert_2 = lc;
}
