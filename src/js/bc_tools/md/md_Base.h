#pragma once
#include "REF.h"
#include "blake2bHasher.h"
#include "commonError.h"
#include "THASH_id.h"
#include "msg.h"
namespace MsgData
{

    struct Base: public Refcountable
    {
        int type;
        Base(int type_):Refcountable("MsgData::Base"),
        type(type_) {}
        virtual ~Base() {}
        virtual void pack(outBuffer& b) const
        {
            MUTEX_INSPECTOR;
            b<<type;

        }
        virtual void unpack(inBuffer& b)
        {
        }
        void unpack2(inBuffer& b)
        {
            MUTEX_INSPECTOR;
            auto t=b.get_PN();
            if(t!=type)
                throw CommonError("if(type!=mtype) %d %d", t, type);
            unpack(b);
        }
        std::string getBuffer() const
        {
            outBuffer o;
            XTRY;
            MUTEX_INSPECTOR;
            pack(o);
            XPASS;
            return o.asString()->container;
        }
        virtual void update(Blake2bHasher& h) const = 0;

        THASH_id getHash() const
        {
            Blake2bHasher h;
            update(h);
            THASH_id r;
            r.container=h.final();
            return r;
        }

    };
}
