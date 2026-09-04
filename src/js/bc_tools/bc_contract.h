#pragma once
#include "cellable.h"
#include "ADDRESS_id.h"
struct bc_contract:  public data_base
{

    bc_contract(Cellable *p):data_base(hsh::bc_contract,p, 0,-1) {}
    std::string name_;
    ADDRESS_id  owner;
    std::string src;
    void pack(outBuffer&b) const final
    {
        data_base::pack(b);
        b<<1;
        b<<name_<<owner<<src;

    }
    void unpack(inBuffer&b) final
    {
        data_base::unpack(b);
        auto v=b.get_PN();
        b>>name_>>owner>>src;

    }
};
