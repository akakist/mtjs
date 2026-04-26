#pragma once

#include "broadcaster.h"

// #include "signedBuffer.h"

#include "listenerBuffered1Thread.h"
#include <map>
#include <rocksdb/db.h>
#include "Events/System/Run/startServiceEvent.h"
#include "Events/Tools/telnetEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Event/bcEvent.h"
#include "root_contract.h"
#include "msg.h"
#include "tr_exec.h"
#include "bigint.h"
#include "TRANSACTION_id.h"
#include "THASH_id.h"
#include "NODE_id.h"
#include "db_to_save.h"
#include <thread>
#include <condition_variable>

#define BROADCAST_ACK_TIMEDOUT_SEC 0.2

    enum timers
    {
        TIMER_BROADCAST_ACK_TIMEDOUT,
        // TIMER_START_HEART_BEAT,
        // TIMER_RESTART_BLOCK,
    };
    struct TIMER_BROADCAST_ACK_TIMEDOUT_cookie: public Refcountable
    {
        SERVICE_id dstService;
        NODE_id dstName_;
        BroadcasterTree::TreeNode tree;
        std::string msg;
        REF_getter<root_data> root=NULL;
        route_t route;

    };

namespace BroadcasterTree
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

        bool ServiceInit(const bcEvent::ServiceInit *e);
        bool GetTransactions(const bcEvent::GetTransactions*e);
        bool InvalidateRoot(const bcEvent::InvalidateRoot*e);
        bool MsgReply(const bcEvent::MsgReply* e, bool fromNetwork);

        bool BroadcastMessage(const bcEvent::BroadcastMessage*e);
        bool SendToChild(const bcEvent::SendToChild*e, bool fromNetwork);
        bool SendToChildAck(const bcEvent::SendToChildAck*e, bool fromNetwork);
        
        void logNode(const char* fmt, ...);



        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();

        // void make_broadcast_message(const std::string & msg);
        // void make_broadcast_message(const std::vector<uint8_t> & msg);
        void make_broadcast_message_to_tree(SERVICE_id dstService,const std::string & msg, const BroadcasterTree::TreeNode& root, const route_t& route);


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

        REF_getter<bcEvent::ServiceInit> conf=nullptr;



    };

}

