#pragma once
#include "cellable.h"
#include "time_types.h"

struct bc_activity: public data_base
{
    bc_activity(Cellable *p): data_base(hsh::bc_activity, p) {}
    std::string name_;   // имя контракта (ссылка на bc_contract)
    std::set<std::string> dbid;
    TIME_mks _update_time;   // момент последнего изменения (мкс)
    TTL_mks ttl;   // внутри микросекунды, наружу отдаётся в секундах
    void update_time(TIME_mks t, Rollback* roll)
    {
        if(t>_update_time)
        {
            _update_time=t;
            setDirty(roll, t);
        }
    }
    size_t size() const { return size_() + name_.size(); }
    void pack(outBuffer& o) const final {
        data_base::pack(o);
        o<<1;
        o<<name_<<dbid<<_update_time.v<<ttl.v;
    }
    void unpack(inBuffer& o) final {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>name_>>dbid>>_update_time.v>>ttl.v;
    }
};