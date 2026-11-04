#pragma once
#include "cellable.h"
#include "ADDRESS_id.h"
struct bc_values: public data_base
{


    bc_values(Cellable *p): data_base(hsh::bc_values,p,0,-1) {
        /// gas
        values["gas_per_token"]    = 1000;
        values["gas_per_byte"]     = 1;
        values["gas_per_jsi_step"] = 10;      // = interrupt_counter
        values["gas_per_hostcall"] = 500;
        values["gas_per_call"]     = 1000;
        values["min_tx_fee"]       = 1;
        values["gas_node_create"]  = 10000;
        values["gas_node_enable"]  = 5000;
        values["gas_node_stake"]   = 2000;
        values["gas_node_update"]  = 1000;
        values["gas_node_disable"] = 5000;

        values["gas_contract_deploy"]  = 10000;
        values["gas_create_base"]      = 100;
        values["ttl_seconds_per_gas"]  = 86400;        
        values["gas_create_address"]   = 1000;

        /// another params
        values["validator_count_percent"] = 50;
        values["validator_min_stake"] = 50;
        values["missed_rounds_max"] = 100 ;
        values["ttl_cache"] = 20 ; /// time to live contract cache and state cache in height
        

    }
    std::map<std::string,int64_t> values;
    std::set<ADDRESS_id> emitters_bin;

    size_t size() const
    {
        M_LOCK(parent->mx);

        size_t sz=size_();
        for(auto &z: values)
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
        auto it=values.find(fee_type);
        if(it!=values.end())
            return it->second;
        throw CommonError("fee '%s' not found", fee_type.c_str());
        return 0;
    }
    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<values<<emitters_bin;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>values>>emitters_bin;
    }

};