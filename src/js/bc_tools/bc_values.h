#pragma once
#include "cellable.h"
#include "ADDRESS_id.h"
struct bc_values: public data_base
{


bc_values(Cellable *p): data_base(hsh::bc_values,p,0,-1) {
        fees["contract_deploy"]=5000;
        fees["contract_transfer"]=1000;
        fees["node_create"]=20000;
        fees["node_update"]=10000;
        fees["node_enable"]=5000;
        fees["node_unstake"]=2000;
        fees["node_stake"]=2000;
        fees["mint"]=95;
        fees["transfer"]=1000;
        fees["cashback"]=200;
    }
    std::map<std::string,uint64_t> fees;
    std::set<ADDRESS_id> emitters_bin;
    int validator_count=5;
    uint64_t validator_minstake=100;

    uint64_t getGas(const std::string &fee_type) const
    {
        auto it=fees.find(fee_type);
        if(it!=fees.end())
            return it->second;
        throw CommonError("fee '%s' not found", fee_type.c_str());
        return 0;
    }
    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<fees<<emitters_bin;
        o<<validator_count<<validator_minstake;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>fees>>emitters_bin;
        o>>validator_count>>validator_minstake;
    }

};
