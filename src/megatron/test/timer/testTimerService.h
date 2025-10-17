#ifndef _______________displayzzPLAYBCNODE___H1INV
#define _______________displayzzPLAYBCNODE___H1INV
#include "broadcaster.h"

#include "listenerBuffered1Thread.h"
#include <map>
#include "Events/System/Run/startServiceEvent.h"



namespace testTimer
{
    class Service:
        public UnknownBase,
    /// ListenerBuffered1Thread - means make single threaded service with own thread.
    /// In this case you don't need guard class members by mutex.
        public ListenerBuffered1Thread,
        public Broadcaster
    {
        bool on_startService(const systemEvent::startService*);
        bool handleEvent(const REF_getter<Event::Base>& e);



    public:
        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();

    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        /// this static constructor is needed for fabric of services.
        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* obj)
        {
            return new Service(id,nm,obj);
        }
        IInstance * iInstance;

    };

}
#endif

