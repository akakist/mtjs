#pragma once
#include "cellable.h"

struct bc_contract_activity: public data_base
{
    bc_contract_activity(Cellable *p): data_base(hsh::bc_contract_activity, p, 0, -1) {}
    std::string name_;   // имя контракта (ссылка на bc_contract)
    size_t size() const { return size_() + name_.size(); }
    void pack(outBuffer& o) const final {
        data_base::pack(o);
        o<<1;
        o<<name_;
    }
    void unpack(inBuffer& o) final {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>name_;
    }
};