#pragma once

#include "cellable.h"
struct Dirty
{
    std::map<REF_getter<data_base>, std::set<REF_getter<fee_calcer>> > dirties;

    void addDirty(const REF_getter<data_base>& d, const REF_getter<fee_calcer>& c)
    {
        dirties[d].insert(c);
    }
    void clear()
    {
        dirties.clear();
    }
};