#pragma once
#include "cellable.h"
#include "ADDRESS_id.h"
struct bc_values: public data_base
{


    bc_values(Cellable *p): data_base(hsh::bc_values,p,0,-1) {
        /// gas
        fees["gas_per_token"]    = 1000;
        fees["gas_per_byte"]     = 1;
        fees["gas_per_jsi_step"] = 10;      // = interrupt_counter
        fees["gas_per_hostcall"] = 500;
        fees["gas_per_call"]     = 1000;
        fees["min_tx_fee"]       = 1;
        fees["gas_node_create"]  = 10000;
        fees["gas_node_enable"]  = 5000;
        fees["gas_node_stake"]   = 2000;
        fees["gas_node_update"]  = 1000;
        fees["gas_node_disable"] = 5000;
        /// another params
        fees["validator_count_percent"] = 50;
        fees["validator_min_stake"] = 50;
        fees["missed_rounds_max"] = 100;
        
        

    }
    std::map<std::string,int64_t> fees;
    std::set<ADDRESS_id> emitters_bin;

    size_t size() const
    {
        M_LOCK(parent->mx);

        size_t sz=size_();
        for(auto &z: fees)
        {
            sz+=z.first.size();
            sz+=sizeof(int64_t);
        }
        for(auto& z: emitters_bin)
        {
            sz+=z.addr.size();
        }
        return sz;
    }

    int64_t getGas(const std::string &fee_type) const
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
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>fees>>emitters_bin;
    }

};