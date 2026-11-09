#pragma once
#include "cellable.h"

struct bc_activity: public data_base
{
    bc_activity(Cellable *p): data_base(hsh::bc_activity, p, 0, 0) {}
    std::string name_;   // имя контракта (ссылка на bc_contract)
    std::set<std::string> dbid;
    time_t _update_time=0;
    int ttl=0;
    void update_time(time_t t, Rollback* roll)
    {
        if(t>_update_time)
        {
            _update_time=t;
            setDirty(roll);
        }
    }
    size_t size() const { return size_() + name_.size(); }
    void pack(outBuffer& o) const final {
        data_base::pack(o);
        o<<1;
        o<<name_<<dbid<<_update_time<<ttl;
    }
    void unpack(inBuffer& o) final {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>name_>>dbid>>_update_time>>ttl;
    }
};