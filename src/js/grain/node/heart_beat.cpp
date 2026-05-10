#include "nodeService.h"
#include "QUORUM.h"
#include "tools_mt.h"
#include "blst_cp.h"
bool Node::Service::HeartBeatRSP(const MsgEvt::HeartBeatRSP* m, const NODE_id & src_node, const route_t& route)
{
    XTRY;

    // logNode("@@ %s",__FUNCTION__);
    auto &hbs=blocks_leader[prev_block_hash].heart_beat_store;
    auto &li=hbs.leader_info;
    if(prev_block_hash!=m->payload_heart_beat->prev_block_hash)
    {
        logErr2("heat beat expired %s %s",prev_block_hash.str().c_str(),m->payload_heart_beat->prev_block_hash.str().c_str());
        return false;
    }


    auto n=root->getNode(m->node_signer,NULL);
    if(!n.valid())
    {
        logErr2("if(!n.valid())");
        return false;
    }
    outBuffer o;
    m->payload_heart_beat->pack(o);
    if(!m->signature.verify(n->bls_pk, blake2b_hash(o.asString()->container).container))
    {
        logNode("if(!sig_check.verify(n->bls_pk, blake2b_hash(mhbr.payload)))");
        return false;
    }
    {
        heart_beat_responce2 hbrs2;
        hbrs2.rsp=m;
        hbrs2.stake=n->total_stake;
        li.responses.insert({m->node_signer,hbrs2});

    }
    BigInt hb_staked=0;
    {
        blst_cpp::AggregateSignature sig_agg;
        std::vector<blst_cpp::PublicKey> pk_agg;
        bool matched=true;
        if(li.responses.empty())
            throw CommonError("if(li.responses.empty())");

        for(auto &z:li.responses)
        {
            sig_agg.add(z.second.rsp->signature);
            auto nn=root->getNode(z.second.rsp->node_signer,NULL);
            pk_agg.push_back(nn->bls_pk);
            hb_staked+=z.second.stake;
        }
    }
    auto pers=(hb_staked.toDouble())/root->getValues(NULL)->total_staked.toDouble();

    if(pers>QUORUM)
    {
        make_leader_certificate();
        if(!li.request_for_transactions_sent)
        {
            logNode("lider approved %s",m->payload_heart_beat->node_leader.container.c_str());
            li.request_for_transactions_sent=true;
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
    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));
    // auto &hbs=blocks[prev_block_hash].heart_beat_store;
    // hbs.clear();
    // auto &li=hbs.leader_info[hbs.node_leader];
    // li.request_for_transactions_sent=false;


    // if(last_access_time_hbZ + HEART_BEAT_TIMEDOUT_SEC<time(NULL))
    // {
    //     logNode("replace leader to %s",hbs.node_leader.container.c_str());
    //     hbs.node_leader=this_node_name;
    // }
    // if(hbs.node_leader.container.empty())
    //     hbs.node_leader=this_node_name;
    // else if(hbs.node_leader!=this_node_name)
    // {
    //     auto old_leader=root->getNode(hbs.node_leader,NULL);
    //     auto my_node=root->getNode(this_node_name,NULL);
    //     if(!old_leader.valid())
    //         hbs.node_leader=this_node_name;
    //     if(!my_node.valid())
    //         throw CommonError("if(!my_node.valid())");
    //     if(old_leader.valid() && my_node.valid())
    //     {
    //         if(old_leader->total_stake<my_node->total_stake)
    //             hbs.node_leader=this_node_name;
    //     }

    // }

    // if(hbs.node_leader==this_node_name)
    {
        REF_getter<MsgEvt::HeartBeatREQ> hb_req=
            new MsgEvt::HeartBeatREQ(prev_block_hash, 
                root->getEpoch(NULL)->epoch, 
                this_node_name);
        // DBG(logNode("broadcast heart beat as leader %s",this_node_name.container.c_str()));
        outBuffer o;
        hb_req->pack(o);
        msg::node_message_ed nm(o.asString()->container,this_node_name,my_sk_ed);
        sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(),ListenerBase::serviceId));
    }

    return;

}
bool Node::Service::DoHeartBeatREQ(const MsgEvt::DoHeartBeatREQ* r, const NODE_id & src_node, const route_t& route)
{
    // logNode("@@ %s",__FUNCTION__);
    last_leader_cert=r->prev_leader_cert;
    do_heart_beat();
    return true;
}


void Node::Service::make_leader_certificate()
{
    auto &hbs=blocks_leader[prev_block_hash].heart_beat_store;
    auto &li=hbs.leader_info;
    REF_getter<MsgEvt::LeaderCertificate>  lc= new MsgEvt::LeaderCertificate();
    if(li.responses.empty())
        return;
    auto msg=li.responses.begin()->second.rsp->payload_heart_beat;
    lc->heart_beat=li.responses.begin()->second.rsp->payload_heart_beat;
    for(auto &r:li.responses)
    {
        lc->agg_sig.add(r.second.rsp->signature);
        auto nn=root->getNode(r.second.rsp->node_signer,NULL);
        lc->nodes.push_back(r.second.rsp->node_signer);
    }

    li.leader_cert_2=lc;
    last_leader_cert=lc;

}
