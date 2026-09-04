#pragma once
#include "cellable.h"
// #include 
struct bc_contract_data:  public data_base
{

    bc_contract_data(Cellable *p):data_base(hsh::bc_contract_data,p, 0,-1) {}
    std::string container;
    void pack(outBuffer&b) const final
    {
        data_base::pack(b);
        b<<1;
        b<<container;

    }
    void unpack(inBuffer&b) final
    {
        data_base::unpack(b);
        auto v=b.get_PN();
        b>>container;

    }
};
