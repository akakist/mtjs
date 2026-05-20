#include "commonError.h"
#include "ioBuffer.h"
#include "blake2bHasher.h"
#include "bigint.h"
#include "REF.h"
#include "bcEvent.h"
#include "mutexInspector.h"
#include "Events/System/timerEvent.h"
#include "nodeService.h"
#include "QUORUM.h"
#include "tools_mt.h"
#include "blst_cp.h"
#include "msg.h"
#include "NODE_id.h"
#include "route_t.h"
#include <cstddef>
#include <vector>
bool Node::Service::HeartBeatRSP(const MsgData::HeartBeatRSP *m, const NODE_id &src_node, const route_t &route)
{
    XTRY;

    auto &hbs = blocks_leader[prev_block_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    if (prev_block_hash_Z != m->payload_heart_beat->prev_block_hash)
    {
        logNode("heat beat expired %s %s", prev_block_hash_Z.str().c_str(), m->payload_heart_beat->prev_block_hash.str().c_str());
        return false;
    }

    auto n = root->getNode(m->node_signer);
    if (!n.valid())
    {
        logErr2("if(!n.valid())");
        return false;
    }
    outBuffer o;
    m->payload_heart_beat->pack(o);
    if (!m->signature.verify(n->bls_pk, blake2b_hash(o.asString()->container).container))
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
                hb_staked += nn->total_stake;
                pk_agg.push_back(nn->bls_pk);
                sig_agg.add(z.second->signature);
            }
        }
    }
    auto pers = (hb_staked.toDouble()) / root->getValues()->total_staked.toDouble();

    if (pers > QUORUM && !li.confirm_leader_sent)
    {
        li.confirm_leader_sent = true;
        {

            REF_getter<MsgData::ConfirmLeaderREQ> rt = new MsgData::ConfirmLeaderREQ();
            rt->hb = m->payload_heart_beat;
            broadcast_MsgEvent(rt.get());
            // msg::node_message_ed nm(rt->getBuffer(), this_node_name, my_sk_ed);
            // sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(), ListenerBase::serviceId));
        }
    }
    XPASS;
    return true;
}

bool Node::Service::HeartBeatREQ(const MsgData::HeartBeatREQ *h, const NODE_id &src_node, const route_t &route)
{
    MUTEX_INSPECTOR;
    // logNode("@@ %s",__FUNCTION__);
    // if(state_Z!=NORMAL)
    //     return true;
    if(CheckState(h, src_node))
        return true;

    sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));

    bool need_replace = false;
    // auto &hbs=blocks_leader[prev_block_hash].heart_beat_store;

    if (prev_block_hash_Z != h->prev_block_hash)
    {
        logNode("invalid root hashe, no answer");
        return true;
    }
    bool need_reply = false;
    if (node_leader_for_client.container.empty())
        node_leader_for_client = h->node_leader;
    if (h->node_leader != node_leader_for_client)
    {
        if (isNodeGreaterOrEqual(h->node_leader, node_leader_for_client))
        {
            node_leader_for_client = h->node_leader;
            need_reply = true;
        }
    }
    else
        need_reply = true;
    if (need_reply)
    {
        REF_getter<MsgData::HeartBeatRSP> hbr = new MsgData::HeartBeatRSP();
        // msg::heart_beat_rsp hba;
        hbr->payload_heart_beat = h;
        hbr->node_signer = this_node_name;
        hbr->signature.sign(my_sk_bls, blake2b_hash(h->getBuffer()).container);

        pass_NodeMsgRSP(hbr.get(),route);
        // auto buf=hbr->getBuffer();
        // auto sig=sign_ed(my_sk_ed,blake2b_hash(buf).container);

        // msg::node_message_ed nme(hbr->getBuffer(), this_node_name, my_sk_ed);
        // logNode("passEvent MsgReply %s",poppedFrontRoute(route).dump().c_str());
        // passEvent(new bcEvent::NodeMsgRSP(this_node_name,sig,buf, poppedFrontRoute(route)));
    }
    return true;
}
bool Node::Service::ConfirmLeaderREQ(const MsgData::ConfirmLeaderREQ *h, const NODE_id &src_node, const route_t &route)

// bool Node::Service::ConfirmLeaderREQ(const MsgData::ConfirmLeaderREQ* h,const std::string &heart_beat_payload, const route_t& route)
{
    MUTEX_INSPECTOR;
    // logNode("@@ %s",__FUNCTION__);
    if(state_Z!=NORMAL)
        return true;

    sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));

    bool need_replace = false;
    // auto &hbs=blocks_leader[prev_block_hash].heart_beat_store;

    if (prev_block_hash_Z != h->hb->prev_block_hash)
    {
        logNode("invalid root hashe, no answer");
        return true;
    }
    bool need_reply = false;
    if (node_leader_for_client.container.empty())
        node_leader_for_client = h->hb->node_leader;
    if (h->hb->node_leader != node_leader_for_client)
    {
        if (isNodeGreaterOrEqual(h->hb->node_leader, node_leader_for_client))
        {
            node_leader_for_client = h->hb->node_leader;
            need_reply = true;
        }
    }
    else
        need_reply = true;

    if (need_reply)
    {
        REF_getter<MsgData::ConfirmLeaderRSP> hbr = new MsgData::ConfirmLeaderRSP();
        // msg::heart_beat_rsp hba;
        hbr->hb = h->hb;
        hbr->node_signer = this_node_name;
        hbr->sig.sign(my_sk_bls, blake2b_hash(h->hb->getBuffer()).container);

        
        // msg::node_message_ed nme(hbr->getBuffer(), this_node_name, my_sk_ed);
        // logNode("passEvent MsgReply %s",poppedFrontRoute(route).dump().c_str());
        pass_NodeMsgRSP(hbr.get(),route);
        // auto buf=hbr->getBuffer();
        // auto sig=sign_ed(my_sk_ed,blake2b_hash(buf).container);
        // passEvent(new bcEvent::NodeMsgRSP(this_node_name,sig,buf, poppedFrontRoute(route)));
    }
    return true;
}

bool Node::Service::ConfirmLeaderRSP(const MsgData::ConfirmLeaderRSP *m, const NODE_id &src_node, const route_t &route)
{
    XTRY;
    if(state_Z!=NORMAL)
        return true;

    // logNode("@@ %s",__FUNCTION__);
    auto &hbs = blocks_leader[prev_block_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    if (prev_block_hash_Z != m->hb->prev_block_hash)
    {
        logErr2("heat beat expired %s %s", prev_block_hash_Z.str().c_str(), m->hb->prev_block_hash.str().c_str());
        return false;
    }

    auto n = root->getNode(m->node_signer);
    if (!n.valid())
    {
        logErr2("if(!n.valid())");
        return false;
    }
    outBuffer o;
    m->hb->pack(o);
    if (!m->sig.verify(n->bls_pk, blake2b_hash(o.asString()->container).container))
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
            pk_agg.push_back(nn->bls_pk);
            hb_staked += nn->total_stake;
        }
    }
    auto pers = (hb_staked.toDouble()) / root->getValues()->total_staked.toDouble();

    if (pers > QUORUM)
    {
        make_leader_certificate();
        if (!li.request_for_transactions_sent)
        {
            logNode("lEAder approved %s", m->hb->node_leader.container.c_str());
            li.request_for_transactions_sent = true;
            do_request_for_transactions(li);
        }
    }
    XPASS;
    return true;
}

void Node::Service::do_heart_beat()
{
    blocks_leader.clear();
    // logNode("@@ %s",__FUNCTION__);
    sendEvent(ServiceEnum::Timer, new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT, NULL, NULL, HEART_BEAT_INTERVAL_SEC, this));
    {
        REF_getter<MsgData::HeartBeatREQ> hb_req =
            new MsgData::HeartBeatREQ(prev_block_hash_Z,
                                     root->getEpoch()->epoch+1,
                                     this_node_name);
        // DBG(logNode("broadcast heart beat as leader %s",this_node_name.container.c_str()));

        broadcast_MsgEvent(hb_req.get());
        // outBuffer o;
        // hb_req->pack(o);

        // msg::node_message_ed nm(o.asString()->container, this_node_name, my_sk_ed);
        // sendEvent(ServiceEnum::BroadcasterTree, new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(), ListenerBase::serviceId));
    }

    return;
}
bool Node::Service::DoHeartBeatREQ(const MsgData::DoHeartBeatREQ *r, const NODE_id &src_node, const route_t &route)
{
        if(state_Z!=NORMAL)
        return true;

    last_leader_cert = r->prev_leader_cert;

    do_heart_beat();
    return true;
}

void Node::Service::make_leader_certificate()
{
    auto &hbs = blocks_leader[prev_block_hash_Z].heart_beat_store;
    auto &li = hbs.leader_info;
    REF_getter<MsgData::LeaderCertificate> lc = new MsgData::LeaderCertificate();
    if (li.ConfirmLeaderRSP_m.empty())
        return;
    auto msg = li.ConfirmLeaderRSP_m.begin()->second->hb;
    lc->heart_beat = li.ConfirmLeaderRSP_m.begin()->second->hb;
    for (auto &r : li.ConfirmLeaderRSP_m)
    {
        lc->agg_sig.add(r.second->sig);
        auto nn = root->getNode(r.second->node_signer);
        lc->nodes.push_back(r.second->node_signer);
    }

    li.leader_cert_2 = lc;
    last_leader_cert = lc;
}
