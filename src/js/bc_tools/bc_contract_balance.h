#pragma once
#include "cellable.h"
struct bc_contract_balance: public data_base
{
    bc_contract_balance(Cellable *p):data_base(hsh::bc_contract_balance,p,0,-1) { amount=0; }
    int64_t amount;
    size_t size() const
    {
        return size_()+sizeof(amount);
    }
    int64_t getAmount() const
    {
        
        return amount;
    }
    void addAmount(int64_t a)
    {
        
        amount += a;
    }
    void subAmount(int64_t a)
    {
        
        amount -= a;
    }
    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<amount;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>amount;
    }
};