#pragma once
#include "cellable.h"
struct bc_granule:  public data_base
{

    bc_granule(Cellable *p):data_base(hsh::bc_granule,p) {}
    std::string container;
    size_t size() const
    {
        

        return  size_()+container.size();
    }
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
