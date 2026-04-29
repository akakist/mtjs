#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <ctime>
#include <map>
#include "transactionCollectorService.h"
#include "tools_mt.h"
#include "events_transactionCollectorService.hpp"
#include "version_mega.h"
#include "tr_exec.h"
#include "CDatabase.h"
#include "QUORUM.h"
#include <SQLiteCpp/Database.h>
#include "init_root.h"



bool TransactionCollector::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;

    return true;
}

bool TransactionCollector::Service::on_timer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool TransactionCollector::Service::on_alarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    return false;
}


bool TransactionCollector::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        switch(ID)
        {
        case bcEventEnum::StartElection:
            return StartElection((const bcEvent::StartElection*)e.get());

        case bcEventEnum::StartCollector:
            return StartCollector((const bcEvent::StartCollector*)e.get());
        case bcEventEnum::Msg:
            return Msg((const bcEvent::Msg*)e.get());
        case bcEventEnum::MsgReply:
            return MsgReply((const bcEvent::MsgReply*)e.get());

        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot*)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit*)e.get());
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer*)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm*)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService*)e.get());
        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor*ev=static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get());
            auto &IDA=ev->e->id;

            switch(IDA)
            {
            case bcEventEnum::Msg:
                return Msg((const bcEvent::Msg*)ev->e.get());
            case bcEventEnum::MsgReply:
                return MsgReply((const bcEvent::MsgReply*)ev->e.get());
            default:
                throw CommonError("unhabdled ev %d %s",IDA, iUtils->genum_name(IDA));
            }
        }
        break;
        case rpcEventEnum::IncomingOnConnector:
        {
            const rpcEvent::IncomingOnConnector*ev=static_cast<const rpcEvent::IncomingOnConnector*>(e.get());
            auto &IDC=ev->e->id;
            switch(IDC)
            {
            case bcEventEnum::Msg:
                return Msg((const bcEvent::Msg*)ev->e.get());
            case bcEventEnum::MsgReply:
                return MsgReply((const bcEvent::MsgReply*)ev->e.get());

            default:
                throw CommonError("unhabdled ev %d %s",IDC, iUtils->genum_name(IDC));
            }
        }
        break;

        default:
            throw CommonError("unhabdled ev %d %s",ID, iUtils->genum_name(ID));
        }



    } catch(std::exception &e)
    {
        logErr2("TransactionCollector std::exception  %s",e.what());
    }
    XPASS;
    return false;
}
#include <regex>

TransactionCollector::Service::~Service()
{
}


TransactionCollector::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
}
bool TransactionCollector::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf=e;
    if(!root.valid())
        root=getRoot(conf->db.get());

    init_root(root);
    return true;
}
bool TransactionCollector::Service::InvalidateRoot(const bcEvent::InvalidateRoot*e)
{
    root=getRoot(conf->db.get());
    init_root(root);
    return true;
}
bool TransactionCollector::Service::StartElection(const bcEvent::StartElection*e)
{
    std::string res;
    int err=conf->db->get_cell("#root_hash#",&res);
    if(!err)
    {
        prev_block_hash.container=res;
    }

    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));
    auto &hbs=heart_beat_store;
    hbs.clear();
    auto &li=hbs.leader_info[hbs.node_leader];
    li.request_for_transactions_sent=false;


    if(last_access_time_hbZ + HEART_BEAT_TIMEDOUT_SEC<time(NULL))
    {
        logNode("replace leader to %s",hbs.node_leader.container.c_str());
        hbs.node_leader=conf->this_node_name;
    }
    if(hbs.node_leader.container.empty())
        hbs.node_leader=conf->this_node_name;
    else if(hbs.node_leader!=conf->this_node_name)
    {
        auto old_leader=root->getNode(hbs.node_leader,NULL);
        auto my_node=root->getNode(conf->this_node_name,NULL);
        if(!old_leader.valid())
            hbs.node_leader=conf->this_node_name;
        if(!my_node.valid())
            throw CommonError("if(!my_node.valid())");
        if(old_leader.valid() && my_node.valid())
        {
            if(old_leader->total_stake<my_node->total_stake)
                hbs.node_leader=conf->this_node_name;
        }

    }

    if(hbs.node_leader==conf->this_node_name)
    {
        msg::heart_beat h;
        h.prev_block_hash=prev_block_hash;
        h.node_leader=conf->this_node_name;
        h.epoch=root->getValues(NULL)->epoch;
        DBG(logNode("TIMER_HEART_BEAT broadcast heart beat as leader %s",this_node_name.container.c_str()));
        msg::node_message_ed nm(h.getBuffer(),conf->this_node_name,conf->my_sk_ed);
        sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::BroadcastMessage(ServiceEnum::TransactionCollector, nm.getBuffer(),ListenerBase::serviceId));
        // make_broadcast_message(h.getBuffer());
        // return;

    }


    return true;
}
bool TransactionCollector::Service::Msg(const bcEvent::Msg*e)
{

    MUTEX_INSPECTOR;

    inBuffer in(e->msg);

    auto p=in.get_PN();
    switch(p)
    {
    case msgid::node_message_ed:
    {
        MUTEX_INSPECTOR;
        // logNode("case msgid::node_message_ed:");
        msg::node_message_ed node_message_ed;
        node_message_ed.unpack(in);

        auto n=root->getNode(node_message_ed.src_node,NULL);
        if(!n.valid())
            throw CommonError("invalid node BBB "+node_message_ed.src_node.container);
        if(!node_message_ed.verify(n->ed_pk))
        {
            throw CommonError("if(!node_message_ed.verify_ed_pk(n->ed_pk))");
        }
        inBuffer in2(node_message_ed.payload);
        auto p2=in2.get_PN();
        switch (p2)
        {
#ifdef KALL            
        case msgid::request_for_transactions:
        {
            MUTEX_INSPECTOR;
            msg::request_for_transactions rft(in2);
            msg::leader_certificate lc(rft.payload_lc);
            msg::heart_beat hb(lc.payload_heart_beat);

            if(!root->verify_lider_certificate(lc))
            {
                logErr2("if(!verify_lider_certificate(rft.payload_lc,node_leader))");
                return true;
            }
            if(node_message_ed.src_node!=hb.node_leader)
            {
                logNode("messag src node != node leader %s %s",node_message_ed.src_node.container.c_str(),hb.node_leader.container.c_str());
                return true;
            }
            sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

            last_access_time_hbZ=time(NULL);


            if(hb.prev_block_hash!=prev_block_hash) /// todo непонятно как нода узнает достоверно, что предложенный hb.prev_block_hash валиден
            {
                logNode("root->getValues(NULL)->epoch<hb.epoch %s %s",root->getValues(NULL)->epoch.toString().c_str(),hb.epoch.toString().c_str());
                if(root->getValues(NULL)->epoch<hb.epoch)
                {
                    logNode("if(root->getValues(NULL)->epoch<hb.epoch)");
                    if(state_Z!=State::SYNCING)
                    {
                        logNode("do_sync()");
                        state_Z=State::SYNCING;
                        last_leader_cert=rft.payload_lc;
                        do_sync();
                        return true;
                    }
                }
                else
                {
                    logNode("invalid epoch, skipping");
                    return true;
                }

            }
            sendEvent(ServiceEnum::TxValidator,new bcEvent::GetTransactions(e->route));
            return true;
        }
        break;
#endif
#ifdef KALL
        case msgid::block_request:
        {

            MUTEX_INSPECTORS("block_request");

            if(state_Z!=State::NORMAL)
                return true;

            // sendEvent(ServiceEnum::Executor,new bcEvent::Msg(nm4.payload,e->route));
            // return true;
            msg::block_request b(in2);
            msg::leader_certificate lc(b.leader_cert);
            if(!root->verify_lider_certificate(b.leader_cert))
                throw CommonError("if(!verify_lider_certificate(b.leader_cert))");

            msg::heart_beat hb(lc.payload_heart_beat);

            if(hb.prev_block_hash!=prev_block_hash)
            {
                if(root->getValues(NULL)->epoch<hb.epoch)
                {
                    //prev_block_hash=hb.prev_block_hash;
                    setBlockId(hb.prev_block_hash);
                    return true;
                }
                logNode("ERROR: block_request block %s, nextblock %s",hb.prev_block_hash.str().c_str(), prev_block_hash.str().c_str());

            }
            {

                auto new_root_hash=execute_block(root,prev_block_hash, b.transaction_bodies,lc.nodes);
                msg::blockZ block;
                block.prev_root_hash=prev_block_hash;
                block.new_root_hash1=new_root_hash;


                block.attachment_hash.container=prepared_block.att_data.hash();

                block.payload_heart_bit=lc.payload_heart_beat;

                msg::block_response br;
                br.node_validator=this_node_name;
                br.payload_block=block.getBuffer();
                br.sign(my_sk_bls);

                msg::node_message_ed nn(br.getBuffer(),this_node_name,my_sk_ed);
                passEvent(new bcEvent::MsgReply(nn.getBuffer(),poppedFrontRoute(e->route)));


            }
        }
        break;        

        break;
#endif
        case msgid::heart_beat:
        {
            MUTEX_INSPECTOR;
            //  logNode("case msgid::heart_beat:");
            msg::heart_beat h;
            h.unpack(in2);
            on_heart_beat(h,node_message_ed.payload, e->route);
        }
        break;
#ifdef KALL
        case msgid::block_accepted_req:
        {
            if(state_Z!=State::NORMAL)
                return true;
            MUTEX_INSPECTORS("block_accepted");
            // logNode("msgid::block_accepted");
            msg::block_accepted_req ba(in2);
            on_block_accepted_req(ba,node_message_ed.src_node, e->route);
            //////////////////////////

        }
        break;
#endif
        default:
            throw CommonError("unhabdled 33p2 %s",msgName(p2));
            break;
        }
    } break;
    default:
        throw CommonError("unhabdled Zp11 %s",msgName(p));
    }
    return true;
}

bool TransactionCollector::Service::on_heart_beat(const msg::heart_beat &h,const std::string &heart_beat_payload, const route_t& route)
{
    MUTEX_INSPECTOR;

    sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

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
        hba.node_signer=conf->this_node_name;
        hba.signature.sign(conf->my_sk_bls, blake2b_hash(heart_beat_payload).container);

        msg::node_message_ed nme(hba.getBuffer(),conf->this_node_name,conf->my_sk_ed);
        // logNode("passEvent MsgReply %s",poppedFrontRoute(route).dump().c_str());
        passEvent(new bcEvent::MsgReply(nme.getBuffer(),poppedFrontRoute(route)));

    }
    return true;
}

// bool TransactionCollector::Service::Msg(const bcEvent::Msg *e)
// {
//     return false;
// }
// bool TransactionCollector::Service::MsgReply(const bcEvent::MsgReply *e)
// {
//     return false;
// }
bool TransactionCollector::Service::MsgReply(const bcEvent::MsgReply* e)
{
    if(e->route.size())
    {
        passEvent(e);
        return true;
    }
    inBuffer in(e->msg);


    auto p=in.get_PN();
    switch(p)
    {
    case msgid::node_message_ed:
    {
        MUTEX_INSPECTOR;
        msg::node_message_ed node_message_ed;
        node_message_ed.unpack(in);
        auto n=root->getNode(node_message_ed.src_node,NULL);
        if(!n.valid())
            throw CommonError("invalid node AAA "+node_message_ed.src_node.container);
        if(!node_message_ed.verify(n->ed_pk))
        {
            throw CommonError("if(!node_message_ed.verify_ed_pk(n->ed_pk))");
        }
        inBuffer in2(node_message_ed.payload);
        auto p2=in2.get_PN();
        switch (p2)
        {
        case msgid::heart_beat_rsp:
        {
            MUTEX_INSPECTOR;
            if(e->route.size())
            {
                passEvent(new bcEvent::MsgReply(e->msg, e->route));
                return true;
            }

            msg::heart_beat_rsp m_heart_beat_rsp;
            m_heart_beat_rsp.unpack(in2);
            on_heart_beat_rsp(m_heart_beat_rsp);

            return true;
        }
        break;
        #ifdef KALL
        case msgid::block_response:
        {
            msg::block_response br(in2);

            on_blockResponse(br);


        }
        break;
        #endif
        #ifdef KALL
        case msgid::response_with_transactions:
        {
            msg::response_with_transactions rwt(in2);
            for(auto& z: rwt.trs)
            {
                THASH_id h=blake2b_hash(z.container);
                transaction_pool_of_leader.insert({h,z});
            }
            auto &hbs=heart_beat_store;
            auto &li=hbs.leader_info[hbs.node_leader];
            li.transaction_responders.insert(node_message_ed.src_node);
            BigInt stake=0;
            for(auto &z :li.transaction_responders)
            {
                auto n=root->getNode(z,NULL);
                stake+=n->total_stake;
            }
            if(stake.toDouble() > root->getValues(NULL)->total_staked.toDouble() * QUORUM)
            {
                do_start_block();
                li.transaction_responders.clear();
            }
            return true;
        }
        break;
        case msgid::block_accepted_rsp:
        {
            msg::block_accepted_rsp bar(in2);
            if(!bar.verify(root->getNode(bar.node_signer,NULL)->bls_pk))
            {
                logErr2("block_accepted_rsp: verify failed");
                return true;
            }
            auto &bp=blocks[prev_block_hash];
            bp.acceptors.insert({bar.node_signer,bar});

            blst_cpp::AggregateSignature agg_sig;
            std::vector<blst_cpp::PublicKey> agg_pk;
            BigInt stake;
            stake=0;
            for(auto &z:bp.acceptors)
            {
                agg_sig.add(z.second.sig_bls);
                auto n=root->getNode(z.first,NULL);
                if(!n.valid())
                    throw CommonError("if(!n.valid())");

                agg_pk.push_back(n->bls_pk);
                stake+=n->total_stake;
            }
            if(!agg_sig.verify(agg_pk,bar.new_root_hash.container))
            {
                logNode("block_accepted_rsp: aggsig !veried");
                return true;
            }
            if(root->getValues(NULL)->total_staked.toDouble()*0.7 < stake.toDouble())
            {
                if(!bp.heart_bit_sent_on_block_accepted_rsp)
                {
                    bp.heart_bit_sent_on_block_accepted_rsp=true;
                    do_heart_beat();

                }
            }
            last_access_time_hbZ=time(NULL);


        }
        break;
        case msgid::get_blocks_rsp:
        {
            msg::get_blocks_rsp r(in2);
            on_get_blocks_rsp(r);
            return true;
        }
        break;
        #endif
        default:
            throw CommonError("unhandled22 p020 %s",msgName(p2));
        }
    }
    break;
    default:
        throw CommonError("unhandled11 p %s",msgName(p));

    }
    return true;
}
void TransactionCollector::Service::on_heart_beat_rsp(const msg::heart_beat_rsp& hbr)
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
            logNode("lеаder approved");
            // li.leader_approved=true;
            li.request_for_transactions_sent=true;
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!AAAA
            // do_request_for_transactions(li);

            // outBuffer o;
            // o<<hbs.node_leader<<m_heart_beat.prev_block_hash;
        }
    }



}

void TransactionCollector::Service::make_leader_certificate()
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


void TransactionCollector::Service::logNode(const char* fmt, ...)
{

    {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stdout,"%ld [TransactionCollector] [%s] [%s] [%s] ", time(NULL), conf->this_node_name.container.c_str(), prev_block_hash.str().c_str(), root->getValues(NULL)->epoch.toString().c_str());
        vfprintf(stdout,fmt, ap);
        fprintf(stdout,"\n");
        va_end(ap);

    }
}
bool TransactionCollector::Service::StartCollector(const bcEvent::StartCollector* e)
{
    std::string res;
    int err=conf->db->get_cell("#root_hash#",&res);
    if(!err)
    {
        prev_block_hash.container=res;
    }

    msg::request_for_transactions rt;
    rt.payload_lc=e->leader_cert;
    msg::node_message_ed nm(rt.getBuffer(),conf->this_node_name,conf->my_sk_ed);
    sendEvent(ServiceEnum::BroadcasterTree,new bcEvent::BroadcastMessage(ServiceEnum::TxValidator, nm.getBuffer(),ListenerBase::serviceId));

    return true;
}

bool TransactionCollector::Service::MsgReply(const bcEvent::MsgReply* e)
{
    if(e->route.size())
    {
        passEvent(e);
        return true;
    }
    inBuffer in(e->msg);


    auto p=in.get_PN();
    switch(p)
    {
    case msgid::node_message_ed:
    {
        MUTEX_INSPECTOR;
        msg::node_message_ed node_message_ed;
        node_message_ed.unpack(in);
        auto n=root->getNode(node_message_ed.src_node,NULL);
        if(!n.valid())
            throw CommonError("invalid node AAA "+node_message_ed.src_node.container);
        if(!node_message_ed.verify(n->ed_pk))
        {
            throw CommonError("if(!node_message_ed.verify_ed_pk(n->ed_pk))");
        }
        inBuffer in2(node_message_ed.payload);
        auto p2=in2.get_PN();
        switch (p2)
        {
#ifdef KALL            
        case msgid::heart_beat_rsp:
        {
            MUTEX_INSPECTOR;
            if(e->route.size())
            {
                passEvent(new bcEvent::MsgReply(e->msg, e->route));
                return true;
            }

            msg::heart_beat_rsp m_heart_beat_rsp;
            m_heart_beat_rsp.unpack(in2);
            on_heart_beat_rsp(m_heart_beat_rsp);

            return true;
        }
        break;
        case msgid::block_response:
        {
            msg::block_response br(in2);

            on_blockResponse(br);


        }
        break;
#endif
        case msgid::response_with_transactions:
        {
            msg::response_with_transactions rwt(in2);
            for(auto& z: rwt.trs)
            {
                THASH_id h=blake2b_hash(z.container);
                transaction_pool_of_leader.insert({h,z});
            }
            auto &hbs=heart_beat_store;
            auto &li=hbs.leader_info[hbs.node_leader];
            li.transaction_responders.insert(node_message_ed.src_node);
            BigInt stake=0;
            for(auto &z :li.transaction_responders)
            {
                auto n=root->getNode(z,NULL);
                stake+=n->total_stake;
            }
            if(stake.toDouble() > root->getValues(NULL)->total_staked.toDouble() * QUORUM)
            {
                do_start_block();
                li.transaction_responders.clear();
            }
            return true;
        }
        break;
#ifdef KALL
        case msgid::block_accepted_rsp:
        {
            msg::block_accepted_rsp bar(in2);
            if(!bar.verify(root->getNode(bar.node_signer,NULL)->bls_pk))
            {
                logErr2("block_accepted_rsp: verify failed");
                return true;
            }
            auto &bp=blocks[prev_block_hash];
            bp.acceptors.insert({bar.node_signer,bar});

            blst_cpp::AggregateSignature agg_sig;
            std::vector<blst_cpp::PublicKey> agg_pk;
            BigInt stake;
            stake=0;
            for(auto &z:bp.acceptors)
            {
                agg_sig.add(z.second.sig_bls);
                auto n=root->getNode(z.first,NULL);
                if(!n.valid())
                    throw CommonError("if(!n.valid())");

                agg_pk.push_back(n->bls_pk);
                stake+=n->total_stake;
            }
            if(!agg_sig.verify(agg_pk,bar.new_root_hash.container))
            {
                logNode("block_accepted_rsp: aggsig !veried");
                return true;
            }
            if(root->getValues(NULL)->total_staked.toDouble()*0.7 < stake.toDouble())
            {
                if(!bp.heart_bit_sent_on_block_accepted_rsp)
                {
                    bp.heart_bit_sent_on_block_accepted_rsp=true;
                    do_heart_beat();

                }
            }
            last_access_time_hbZ=time(NULL);


        }
        break;
        case msgid::get_blocks_rsp:
        {
            msg::get_blocks_rsp r(in2);
            on_get_blocks_rsp(r);
            return true;
        }
        break;
#endif
        default:
            throw CommonError("unhandled22 p020 %s",msgName(p2));
        }
    }
    break;
    default:
        throw CommonError("unhandled11 p %s",msgName(p));

    }
    return true;
}


void registerTransactionCollectorService(const char* pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::TransactionCollector,"TransactionCollector",getEvents_transactionCollectorService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::TransactionCollector,TransactionCollector::Service::construct,"TransactionCollector");
        regEvents_transactionCollectorService();
    }

    XPASS;
}




