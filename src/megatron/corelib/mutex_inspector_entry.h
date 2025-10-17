#pragma once

/// used in MutexInspector

#include <string>
struct mutex_inspector_entry
{
    struct iint
    {
        iint()
        {
        }
        std::string file;
        int line;
    };
    const char * f;
    const char * func;

    int l;
    time_t t;
    std::string __s;
    mutex_inspector_entry():f(NULL),func(NULL),l(0),t(0) {}
};

