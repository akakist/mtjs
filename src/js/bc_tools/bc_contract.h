#pragma once
#include "cellable.h"
#include "ADDRESS_id.h"
struct bc_contract:  public data_base
{
    bc_contract(Cellable *p):data_base(hsh::bc_contract,p, 0,-1) {}
    std::string name_;
    std::set<ADDRESS_id> owners;
    std::string src;
    size_t size() const
    {
        
        size_t sz = size_() + name_.size() + src.size();
        for (auto& o : owners) sz += o.addr.size();
        return sz;
    }
    bool isOwner(const ADDRESS_id& a) const
    {
        
        return owners.count(a) != 0;
    }
    void pack(outBuffer&b) const final
    {
        data_base::pack(b);
        b<<1;
        b<<name_<<owners<<src;
    }
    void unpack(inBuffer&b) final
    {
        data_base::unpack(b);
        auto v=b.get_PN();
        b>>name_>>owners>>src;
    }
};