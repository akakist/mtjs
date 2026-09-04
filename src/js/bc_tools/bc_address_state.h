#pragma once
#include "cellable.h"
struct bc_address_state: public data_base
{

    bc_address_state(Cellable* p): data_base(hsh::bc_address_state,p, 0,-1) {
        nonce=0;
        balance=0;
    }
    uint64_t balance;
    uint64_t nonce;
    private:
    public:
    uint64_t getNonce()
    {
        M_LOCK(parent->mx);
        return nonce;
    }
    void incNonce()
    {
        M_LOCK(parent->mx);
        nonce+=1;
    }
    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<nonce;
        o<<balance;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>nonce;
        o>>balance;
    }

};
