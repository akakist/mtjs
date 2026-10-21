#pragma once
#include "REF.h"

struct Clock : public Refcountable {
    Clock() : Refcountable("Clock") {}
    virtual ~Clock() = default;
    virtual int64_t current_time() = 0;
    virtual int64_t current_block_height() = 0;
};

