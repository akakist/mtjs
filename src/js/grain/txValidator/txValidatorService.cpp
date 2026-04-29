#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <ctime>
#include <map>
#include "txValidatorService.h"
#include "tools_mt.h"
#include "events_txValidatorService.hpp"
#include "version_mega.h"
#include "tr_exec.h"
#include "CDatabase.h"
#include <SQLiteCpp/Database.h>
#include "init_root.h"



bool TxValidator::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;

    return true;
}

bool TxValidator::Service::on_timer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool TxValidator::Service::on_alarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    return false;
}


bool TxValidator::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        switch(ID)
        {
        case bcEventEnum::Msg:
            return Msg((const bcEvent::Msg*)e.get());
        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot*)e.get());
        case bcEventEnum::GetTransactions:
            return GetTransactions((const bcEvent::GetTransactions*)e.get());
        case bcEventEnum::ClientMsg:
            return ClientMsg((const bcEvent::ClientMsg*)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit*)e.get());
        case bcEventEnum::AddTx:
            return AddTx((const bcEvent::AddTx*)e.get());
        case bcEventEnum::TxValidatorStart:
            return TxValidatorStart((const bcEvent::TxValidatorStart*)e.get());
        case bcEventEnum::TxValidatorStop:
            return TxValidatorStop((const bcEvent::TxValidatorStop*)e.get());
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
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg*)ev->e.get());
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
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg*)ev->e.get());

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
        logErr2("TxValidator std::exception  %s",e.what());
    }
    XPASS;
    return false;
}
#include <regex>

TxValidator::Service::~Service()
{
}


TxValidator::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
}

bool TxValidator::Service::AddTx(const bcEvent::AddTx *e)
{

    return true;
}
bool TxValidator::Service::TxValidatorStart(const bcEvent::TxValidatorStart *e)
{
    // db=e->db;
    // is_working = true;
    // if(!root.valid())
    //     root=getRoot(db.get());

    // init_root(root);

    return true;
}
bool TxValidator::Service::TxValidatorStop(const bcEvent::TxValidatorStop *e)
{
    is_working = false;
    return true;
}
bool TxValidator::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf=e;
    if(!root.valid())
        root=getRoot(conf->db.get());

    init_root(root);
    return true;
}
bool TxValidator::Service::GetTransactions(const bcEvent::GetTransactions*e)
{
    msg::response_with_transactions rwt;
    for(auto& z: transaction_pool_verified)
    {
        rwt.trs.push_back(z.second);
    }
    transaction_pool_verified.clear();
    msg::node_message_ed nm(rwt.getBuffer(),conf->this_node_name,conf->my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(e->route)));

    return true;
}
bool TxValidator::Service::InvalidateRoot(const bcEvent::InvalidateRoot*e)
{
    root=getRoot(conf->db.get());
    init_root(root);
    return true;
}

bool TxValidator::Service::ClientMsg(const bcEvent::ClientMsg*e)
{


    MUTEX_INSPECTOR;
    inBuffer in(e->msg);

    auto p=in.get_PN();
    THASH_id hash;
    hash=blake2b_hash(e->msg);
    switch(p)
    {
    case msgid::user_message_req:
    {
        MUTEX_INSPECTOR;
        std::optional<std::string> err;
        // sendEvent(ServiceEnum::TxValidator,new bcEvent::AddTx(e,this));
        msg::user_message_req um(in);
        if(!err && !um.verify())
        {
            err="verify failed";
            // return true;

        }
        BigInt nonce=0;
        // logErr2("getUser %s",base62::encode(um.address_pk_ed).c_str());
        if(!err)
        {
            auto u=root->getUser(um.address_pk_ed,NULL);
            if(u.valid())
            {
                nonce=u->nonce;
            }
        }

        // logErr2("um.nonce %s",um.nonce.toString().c_str());
        if(!err && nonce!=um.nonce)
        {
            err="invalid_nonce "+ nonce.toString()+" != "+um.nonce.toString();
        }
        if(!err)
        {
            THASH_id h=blake2b_hash(e->msg);
            TRANSACTION_body t;
            t.container=e->msg;
            transaction_pool_verified.insert({h,t});
        }
            // addToTransactionToPool(e->msg);
        msg::transaction_added_rsp tr;
        tr.err=err.has_value();
        tr.err_str=err?*err:"transaction added to pool";
        tr.tx_hash=blake2b_hash(e->msg);
        // msg::node_message_ed nm(tr.getBuffer(),this_node_name,my_sk_ed);
        passEvent(new bcEvent::ClientMsgReply(hash, tr.getBuffer(),poppedFrontRoute(e->route)));
        return true;
    }
    break;
    default:
        throw CommonError("unhandled msgid e. ff %d",p);
    }

    return true;
}
// #ifdef KALL
bool TxValidator::Service::Msg(const bcEvent::Msg*e)
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
        case msgid::request_for_transactions:
        {
            MUTEX_INSPECTOR;
    std::string res;
    int err=conf->db->get_cell("#root_hash#",&res);
    if(!err)
    {
        prev_block_hash.container=res;
    }
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
            // sendEvent(ServiceEnum::Timer,new timerEvent::ResetAlarm(timers::TIMER_START_HEART_BEAT,NULL, NULL,HEART_BEAT_INTERVAL_SEC,this));

            // last_access_time_hbZ=time(NULL);


            // if(hb.prev_block_hash!=prev_block_hash) /// todo непонятно как нода узнает достоверно, что предложенный hb.prev_block_hash валиден
            // {
            //     logNode("root->getValues(NULL)->epoch<hb.epoch %s %s",root->getValues(NULL)->epoch.toString().c_str(),hb.epoch.toString().c_str());
            //     if(root->getValues(NULL)->epoch<hb.epoch)
            //     {
            //         logNode("if(root->getValues(NULL)->epoch<hb.epoch)");
            //         if(state_Z!=State::SYNCING)
            //         {
            //             logNode("do_sync()");
            //             state_Z=State::SYNCING;
            //             last_leader_cert=rft.payload_lc;
            //             do_sync();
            //             return true;
            //         }
            //     }
            //     else
            //     {
            //         logNode("invalid epoch, skipping");
            //         return true;
            //     }

            // }
            // sendEvent(ServiceEnum::TxValidator,new bcEvent::GetTransactions(e->route));
                msg::response_with_transactions rwt;
                for(auto& z: transaction_pool_verified)
                {
                    rwt.trs.push_back(z.second);
                }
                transaction_pool_verified.clear();
                msg::node_message_ed nm(rwt.getBuffer(),conf->this_node_name,conf->my_sk_ed);
                passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(e->route)));

            return true;
        }
        break;
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
        case msgid::heart_beat:
        {
            MUTEX_INSPECTOR;
            //  logNode("case msgid::heart_beat:");
            msg::heart_beat h;
            h.unpack(in2);
            on_heart_beat(h,node_message_ed.payload, e->route);
        }
        break;
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
void TxValidator::Service::logNode(const char* fmt, ...)
{

    {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stdout,"%ld [TxValidator] [%s] [%s] ", time(NULL), conf->this_node_name.container.c_str(), root->getValues(NULL)->epoch.toString().c_str());
        vfprintf(stdout,fmt, ap);
        fprintf(stdout,"\n");
        va_end(ap);

    }
}

// #endif
void registerTxValidatorService(const char* pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::TxValidator,"TxValidator",getEvents_txValidatorService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::TxValidator,TxValidator::Service::construct,"TxValidator");
        regEvents_txValidatorService();
    }
    XPASS;
}




