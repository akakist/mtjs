#pragma once
#include <quickjs.h>
#include <iostream>
#include <string>
#include "common/mtjs_opaque.h"
#include "common/timerTask.h"
class Timeout {
public:
    REF_getter<TimerTask> timerTask;
    REF_getter<RCF> rcf;
    Timeout(const REF_getter<TimerTask> & tt, const REF_getter<RCF> & r)
        : timerTask(tt), rcf(r) {
    }
    ~Timeout()
    {
    }

};

