#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <time.h>
#include <map>
#include "txValidatorService.h"
#include "tools_mt.h"
#include "tree.h"
#include "events_txValidatorService.hpp"
#include "version_mega.h"
#include "tr_exec.h"
#include "CDatabase.h"
#include "s_ed.h"
#include "QUORUM.h"
#include <SQLiteCpp/Database.h>
#include "CDatabase.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
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




