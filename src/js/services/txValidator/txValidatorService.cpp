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


void TxValidator::Service::validator()
{
    while(1)
    {
        if(iUtils->isTerminating())
            return;
        {
            M_LOCKC(mx);
            if(dirty_pool.size()==0 || !is_working)
                condvar.wait();
        }
        while(is_working)
        {
            {
                M_LOCKC(mx);
                if(dirty_pool.size()==0 || !is_working)
                    condvar.wait();
            }

            REF_getter<bcEvent::ClientMsg> m(NULL);
            {
                M_LOCKC(mx);
                if(dirty_pool.size())
                {
                    m=*dirty_pool.begin();
                    dirty_pool.pop_front();
                }

            }
            if(m.valid())
            {

            }
        }

    }
}

bool TxValidator::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;
    _validator=std::thread([this] {
        validator();
    });

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
    _validator.join();
}


TxValidator::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins), condvar(mx)
{
}

bool TxValidator::Service::AddTx(const bcEvent::AddTx *e)
{
    {
        M_LOCKC(mx);
        dirty_pool.push_back(e->msg);
    }

    return true;
}
bool TxValidator::Service::TxValidatorStart(const bcEvent::TxValidatorStart *e)
{
    db=e->db;
    is_working = true;
    if(!root.valid())
        root=getRoot(db.get());

    init_root(root);

    condvar.signal();
    return true;
}
bool TxValidator::Service::TxValidatorStop(const bcEvent::TxValidatorStop *e)
{
    is_working = false;
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




