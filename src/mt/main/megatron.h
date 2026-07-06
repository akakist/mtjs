#pragma once
#include "IConfigObj.h"
#include <stdlib.h>
#include "IInstance.h"
struct megatron
{
private:
    IConfigObj* config;
public:
    IInstance *iInstance;
    ~megatron();
    megatron();
    void run(
        int argc, char* argv[]
    );
#if !defined __MOBILE__

    static bool findProcess(const char* process);
#endif

};

