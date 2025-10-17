#pragma once
#include "IUtils.h"
#include "commonError.h"
class st_log
{

public:
    st_log(const std::string& keyword)
    {
        XTRY;
        iUtils->pushLogPrefix(keyword);
        XPASS;
    }
    ~st_log()
    {
        iUtils->popLogPrefix();
    }
};
#ifdef WITH_SLOG
#define S_LOG(a) st_log aaazzz(a)
#else
#define S_LOG(a)
#endif
