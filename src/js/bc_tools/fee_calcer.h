#pragma once

#include "REF.h"
#include "bigint.h"


struct fee_calcer: public Refcountable
{
private:
    BigInt fee;
public:
    void add(const BigInt &f)
    {
        fee+=f;
    }
    BigInt& get_fee()
    {
        return fee;
    }

};
