#pragma once

#include "broadcaster.h"


#include "listenerBuffered1Thread.h"
#include <map>
#include <rocksdb/db.h>
#include "Events/System/Run/startServiceEvent.h"
#include "Events/Tools/telnetEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "root_contract.h"
#include "msg.h"
#include "TRANSACTION_id.h"
#include "THASH_id.h"
#include <thread>



namespace TxValidator
{
    enum timers
    {
    };
    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster
    {
        bool on_startService(const systemEvent::startService*);
        bool on_timer(const timerEvent::TickTimer*);
        bool on_alarm(const timerEvent::TickAlarm*);
        bool handleEvent(const REF_getter<Event::Base>& e);

        bool AddTx(const bcEvent::AddTx *e);
        bool TxValidatorStart(const bcEvent::TxValidatorStart *e);
        bool TxValidatorStop(const bcEvent::TxValidatorStop *e);
        bool ServiceInit(const bcEvent::ServiceInit *e);
        bool ClientMsg(const bcEvent::ClientMsg*e);
        bool GetTransactions(const bcEvent::GetTransactions*e);
        bool InvalidateRoot(const bcEvent::InvalidateRoot*e);
        bool Msg(const bcEvent::Msg*e);


        void logNode(const char* fmt, ...);

        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();



    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* obj)
        {
            XTRY;
            return new Service(id,nm,obj);
            XPASS;
        }

        std::map<THASH_id, TRANSACTION_body>  transaction_pool_verified;

        // std::thread _validator;


        // void validator();
        // MutexC mx;
        // Condition condvar;

        // std::deque< REF_getter<bcEvent::ClientMsg > > dirty_pool;
        bool is_working=false;
        // std::string last_block_hash;
        REF_getter<root_data> root=NULL;
        // REF_getter<IDatabase> db=nullptr;
        BLOCK_id prev_block_hash;

        REF_getter<bcEvent::ServiceInit> conf=nullptr;



    };

}

