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
        case bcEventEnum::AddTxREQ:
            return AddTxREQ((const bcEvent::AddTxREQ*)e.get());
        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot*)e.get());
        case bcEventEnum::ClientMsg:
            return ClientMsg((const bcEvent::ClientMsg*)e.get());
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
            case bcEventEnum::AddTxREQ:
                return AddTxREQ((const bcEvent::AddTxREQ*)ev->e.get());
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
            case bcEventEnum::AddTxREQ:
                return AddTxREQ((const bcEvent::AddTxREQ*)ev->e.get());
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

bool TxValidator::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    MUTEX_INSPECTOR;
    conf=e;
    if(!root.valid())
        root=getRoot(conf->db.get());

    init_root(root);
    return true;
}
bool TxValidator::Service::InvalidateRoot(const bcEvent::InvalidateRoot*e)
{
    MUTEX_INSPECTOR;
    root=getRoot(conf->db.get());
    init_root(root);
    return true;
}
bool TxValidator::Service::AddTxREQ(const bcEvent::AddTxREQ*e)
{
    MUTEX_INSPECTOR;
    if(!e->tx->verify())
    {

    }
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
    // case msgid::user_message_req:
    // {
    //     MUTEX_INSPECTOR;
    //     std::optional<std::string> err;
    //     msg::user_message_req um(in);
    //     if(!err && !um.verify())
    //     {
    //         err="verify failed";
    //         // return true;

    //     }
    //     if(!err)
    //         sendEvent(ServiceEnum::Node,new bcEvent::PutTransactionREQ(e->msg,this));
    //     msg::transaction_added_rsp tr;
    //     tr.err=err.has_value();
    //     tr.err_str=err?*err:"transaction added to pool";
    //     tr.tx_hash=blake2b_hash(e->msg);
    //     passEvent(new bcEvent::ClientMsgReply(hash, tr.getBuffer(),poppedFrontRoute(e->route)));
    //     return true;
    // }
    // break;
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




