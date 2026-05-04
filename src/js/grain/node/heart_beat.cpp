#include "nodeService.h"
#include "QUORUM.h"
#include "tools_mt.h"
#include "blst_cp.h"
bool Node::Service::HeartBeatRSP(const MsgEvent::HeartBeatRSP* m, const NODE_id & src_node, const route_t& route)
{
    // msg::heart_beat m_heart_beat(m->payload_heart_beat);

    auto &hbs=heart_beat_store;
    auto &li=hbs.leader_info[hbs.node_leader];
    // if(li.responses.count(m->node_signer))
    // {
    //     return;
    // }
    if(prev_block_hash!=m->payload_heart_beat->prev_block_hash)
    {
        logErr2("heat beat expired %s %s",prev_block_hash.str().c_str(),m->payload_heart_beat->prev_block_hash.str().c_str());
        return false;
    }

    // li.respons.insert(m_heart_beat_rsp.node_signer);

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
            // if(*z.second.rsp->payload_heart_beat.get() != * m->payload_heart_beat.get())
            // {
            //     throw CommonError("r.rsp.payload != m_heart_beat_rsp.payload");
            // }
            sig_agg.add(z.second.rsp->signature);
            auto nn=root->getNode(z.second.rsp->node_signer,NULL);
            pk_agg.push_back(nn->bls_pk);
            hb_staked+=z.second.stake;
        }
        // if(!sig_agg.verify(pk_agg, blake2b_hash(m->payload_heart_beat).container))
        // {
        //     logNode("aggig veriify fail ! %s %d",__FILE__,__LINE__);
        //     return false;
        //     // logNode("aggig veriify ok");
        // }
    }
    auto pers=(hb_staked.toDouble())/root->getValues(NULL)->total_staked.toDouble();

    if(pers>QUORUM)
    {
        // logErr2("if(pers>QUORUM)");
        make_leader_certificate();
        if(!li.request_for_transactions_sent)
        {
            logNode("lider approved");
            // li.leader_approved=true;
            li.request_for_transactions_sent=true;
            do_request_for_transactions(li);

            // outBuffer o;
            // o<<hbs.node_leader<<m_heart_beat.prev_block_hash;
        }
    }

    return true;

}

bool Node::Service::HeartBeatREQ(const MsgEvent::HeartBeatREQ* h,const std::string &heart_beat_payload, const route_t& route)
{
    MUTEX_INSPECTOR;

    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

    bool need_reply=false;
    bool need_replace=false;
    // logNode("heart beat received from leader %s last block %s",h.node_leader.c_str(),h.last_block.toString().c_str());
    auto &hbs=heart_beat_store;

    if(hbs.node_leader==h->node_leader)
    {
        need_reply=true;
    }
    else
    {
        auto old_leader=root->getNode(hbs.node_leader,NULL);
        auto new_leader=root->getNode(h->node_leader,NULL);
        if(old_leader.valid() &&  new_leader.valid())
        {
            if(new_leader->total_stake>old_leader->total_stake)
            {
                hbs.node_leader=h->node_leader;
                need_reply=true;
            }
        }
    }
    auto &li=hbs.leader_info[hbs.node_leader];
    if(need_reply)
    {
        last_access_time_hbZ=time(NULL);
        REF_getter<MsgEvent::HeartBeatRSP> hbr=new MsgEvent::HeartBeatRSP();
        // msg::heart_beat_rsp hba;
        hbr->payload_heart_beat=h;
        hbr->node_signer=this_node_name;
        hbr->signature.sign(my_sk_bls, blake2b_hash(heart_beat_payload).container);

        msg::node_message_ed nme(hbr->getBuffer(),this_node_name,my_sk_ed);
        // logNode("passEvent MsgReply %s",poppedFrontRoute(route).dump().c_str());
        passEvent(new bcEvent::MsgReply(nme.getBuffer(),poppedFrontRoute(route)));

    }
    return true;
}

void Node::Service::do_heart_beat()
{
    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));
    auto &hbs=heart_beat_store;
    hbs.clear();
    auto &li=hbs.leader_info[hbs.node_leader];
    li.request_for_transactions_sent=false;


    if(last_access_time_hbZ + HEART_BEAT_TIMEDOUT_SEC<time(NULL))
    {
        logNode("replace leader to %s",hbs.node_leader.container.c_str());
        hbs.node_leader=this_node_name;
    }
    if(hbs.node_leader.container.empty())
        hbs.node_leader=this_node_name;
    else if(hbs.node_leader!=this_node_name)
    {
        auto old_leader=root->getNode(hbs.node_leader,NULL);
        auto my_node=root->getNode(this_node_name,NULL);
        if(!old_leader.valid())
            hbs.node_leader=this_node_name;
        if(!my_node.valid())
            throw CommonError("if(!my_node.valid())");
        if(old_leader.valid() && my_node.valid())
        {
            if(old_leader->total_stake<my_node->total_stake)
                hbs.node_leader=this_node_name;
        }

    }

    if(hbs.node_leader==this_node_name)
    {
        REF_getter<MsgEvent::HeartBeatREQ> hb_req=
            new MsgEvent::HeartBeatREQ(prev_block_hash, 
                root->getValues(NULL)->epoch, 
                this_node_name);
        DBG(logNode("TIMER_HEART_BEAT broadcast heart beat as leader %s",this_node_name.container.c_str()));
        outBuffer o;
        hb_req->pack(o);
        msg::node_message_ed nm(o.asString()->container,this_node_name,my_sk_ed);
        sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::BroadcastMessage(ServiceEnum::Node, nm.getBuffer(),ListenerBase::serviceId));
    }

    return;

}

void Node::Service::make_leader_certificate()
{
    auto &hbs=heart_beat_store;
    auto &li=hbs.leader_info[hbs.node_leader];
    REF_getter<MsgEvent::LeaderCertificate>  lc= new MsgEvent::LeaderCertificate();
    // bls::Signature sig_agg;
    // std::vector<blst_cpp::PublicKey> agg_pk;
    if(li.responses.empty())
        return;
    auto msg=li.responses.begin()->second.rsp->payload_heart_beat;
    // auto h=blake2b_hash(msg);
    lc->heart_beat=li.responses.begin()->second.rsp->payload_heart_beat;
    for(auto &r:li.responses)
    {
        // if(r.second.rsp->payload_heart_beat != msg)
        // {
        //     throw CommonError("r.rsp.payload != m_heart_beat_rsp.payload");
        //     return;
        // }
        lc->agg_sig.add(r.second.rsp->signature);
        auto nn=root->getNode(r.second.rsp->node_signer,NULL);
        // agg_pk.push_back(nn->bls_pk);
        lc->nodes.push_back(r.second.rsp->node_signer);
    }
    // if(!lc->agg_sig.verify(agg_pk,h.container))
    //     throw CommonError("if(!lc.agg_sig.verify(lc.agg_pk))");

    li.leader_cert=lc;

}
