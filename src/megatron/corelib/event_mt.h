#pragma once


#include "REF.h"
#include "EVENT_id.h"
#include "route_t.h"
/// base class for events
namespace Event
{
    class Base: public Refcountable
    {
    public:
        /// тип евента
        const EVENT_id id;
        virtual ~Base() {}
        /// маршрут
        route_t route;
        Base(const EVENT_id&_id): id(_id) {}
        Base(const EVENT_id& _id,const route_t &_route):id(_id),route(_route) {}

        /// pack/unpack to outBuffer/inBuffer, used for RPC
        virtual void pack(outBuffer& b)const =0;
        virtual void unpack(inBuffer& b)=0;

    };


/// base class for local events (without RPC)
    class NoPacked:public Base
    {
    public:
        NoPacked(const EVENT_id&_id): Base(_id) {}
        NoPacked(const EVENT_id& _id, const route_t &_route):Base(_id,_route) {}
        void pack(outBuffer& ) const final;
        void unpack(inBuffer& ) final;

    };

}





