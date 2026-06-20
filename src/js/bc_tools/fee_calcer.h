#pragma once

#include "REF.h"
#include "bigint.h"
#include "ADDRESS_id.h"

struct fee_calcer: public Refcountable
{
private:
    BigInt fee;
public:
    fee_calcer():Refcountable("feecalcer") {}
    void add(const BigInt &f)
    {
        fee+=f;
    }
    BigInt& get_fee()
    {
        return fee;
    }
    void reset()
    {
        fee=0;
    }

};


struct _feeCalcers
{
    std::map<ADDRESS_id, REF_getter<fee_calcer>> calcers;
    REF_getter<fee_calcer> get(const ADDRESS_id &pk)
    {
        auto it=calcers.find(pk);
        if(it==calcers.end())
        {
            calcers.insert({pk,new fee_calcer});
            it=calcers.find(pk);
            if(it==calcers.end())
                throw CommonError("if(it==calcers.end())");

        }
        return it->second;
    }
    void clear()
    {
        calcers.clear();
    }
};
