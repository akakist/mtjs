#include "nodeService.h"
#include "QUORUM.h"
#include "tools_mt.h"
#include "blst_cp.h"
void Node::Service::on_heart_beat_rsp(const msg::heart_beat_rsp& hbr)
{
    msg::heart_beat m_heart_beat(hbr.payload_heart_beat);

    auto &hbs=heart_beat_store;
    auto &li=hbs.leader_info[hbs.node_leader];
    // if(li.responses.count(hbr.node_signer))
    // {
    //     return;
    // }
    if(prev_block_hash!=m_heart_beat.prev_block_hash)
    {
        logErr2("heat beat expired %s %s",prev_block_hash.str().c_str(),m_heart_beat.prev_block_hash.str().c_str());
        return;
    }

    // li.respons.insert(m_heart_beat_rsp.node_signer);

    auto n=root->getNode(hbr.node_signer,NULL);
    if(!n.valid())
    {
        logErr2("if(!n.valid())");
        return;
    }

    if(!hbr.signature.verify(n->bls_pk, blake2b_hash(hbr.payload_heart_beat).container))
    {
        logNode("if(!sig_check.verify(n->bls_pk, blake2b_hash(mhbr.payload)))");
        return;
    }
    {
        heart_beat_responce2 hbrs2;
        hbrs2.rsp=hbr;
        hbrs2.stake=n->total_stake;
        li.responses.insert({hbr.node_signer,hbrs2});

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
            if(z.second.rsp.payload_heart_beat != hbr.payload_heart_beat)
            {
                throw CommonError("r.rsp.payload != m_heart_beat_rsp.payload");
                return;
            }
            sig_agg.add(z.second.rsp.signature);
            auto nn=root->getNode(z.second.rsp.node_signer,NULL);
            pk_agg.push_back(nn->bls_pk);
            hb_staked+=z.second.stake;
        }
        if(!sig_agg.verify(pk_agg, blake2b_hash(hbr.payload_heart_beat).container))
        {
            logNode("aggig veriify fail ! %s %d",__FILE__,__LINE__);
            return;
            // logNode("aggig veriify ok");
        }
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



}

bool Node::Service::on_heart_beat(const msg::heart_beat &h,const std::string &heart_beat_payload, const route_t& route)
{
    MUTEX_INSPECTOR;

    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

    bool need_reply=false;
    bool need_replace=false;
    // logNode("heart beat received from leader %s last block %s",h.node_leader.c_str(),h.last_block.toString().c_str());
    auto &hbs=heart_beat_store;

    if(hbs.node_leader==h.node_leader)
    {
        need_reply=true;
    }
    else
    {
        auto old_leader=root->getNode(hbs.node_leader,NULL);
        auto new_leader=root->getNode(h.node_leader,NULL);
        if(old_leader.valid() &&  new_leader.valid())
        {
            if(new_leader->total_stake>old_leader->total_stake)
            {
                hbs.node_leader=h.node_leader;
                need_reply=true;
            }
        }
    }
    auto &li=hbs.leader_info[hbs.node_leader];
    if(need_reply)
    {
        last_access_time_hbZ=time(NULL);

        msg::heart_beat_rsp hba;
        hba.payload_heart_beat=heart_beat_payload;
        hba.node_signer=this_node_name;
        hba.signature.sign(my_sk_bls, blake2b_hash(heart_beat_payload).container);

        msg::node_message_ed nme(hba.getBuffer(),this_node_name,my_sk_ed);
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
        msg::heart_beat h;
        h.prev_block_hash=prev_block_hash;
        h.node_leader=this_node_name;
        h.epoch=root->getValues(NULL)->epoch;
        DBG(logNode("TIMER_HEART_BEAT broadcast heart beat as leader %s",this_node_name.container.c_str()));
        make_broadcast_message(h.getBuffer());
        // return;

    }

    return;

}

void Node::Service::make_leader_certificate()
{
    auto &hbs=heart_beat_store;
    auto &li=hbs.leader_info[hbs.node_leader];
    msg::leader_certificate lc;
    // bls::Signature sig_agg;
    std::vector<blst_cpp::PublicKey> agg_pk;
    if(li.responses.empty())
        return;
    auto msg=li.responses.begin()->second.rsp.payload_heart_beat;
    auto h=blake2b_hash(msg);
    lc.payload_heart_beat=msg;
    for(auto &r:li.responses)
    {
        if(r.second.rsp.payload_heart_beat != msg)
        {
            throw CommonError("r.rsp.payload != m_heart_beat_rsp.payload");
            return;
        }
        lc.agg_sig.add(r.second.rsp.signature);
        auto nn=root->getNode(r.second.rsp.node_signer,NULL);
        agg_pk.push_back(nn->bls_pk);
        lc.nodes.push_back(r.second.rsp.node_signer);
    }
    if(!lc.agg_sig.verify(agg_pk,h.container))
        throw CommonError("if(!lc.agg_sig.verify(lc.agg_pk))");

    li.leader_cert=lc.getBuffer();

}
