#include "_QUERY.h"
#include "IUtils.h"
#include "commonError.h"
#ifndef _MSC_VER
#endif
#include "mutexInspector.h"


QUERY::QUERY(const std::string &q): query_(q)
{
}
QUERY::QUERY(const char*q): query_(q)
{
}

QUERY::~QUERY()
{
}



QUERY& QUERY::operator << (const std::string &s)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(s);
    return *this;
}

QUERY& QUERY::operator << (const int8_t i)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(std::to_string(i));
    return *this;
}

QUERY& QUERY::operator << (const uint8_t i)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(std::to_string(i));
    return *this;
}
QUERY& QUERY::operator << (const uint16_t i)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(std::to_string(i));
    return *this;
}
QUERY& QUERY::operator << (const int16_t i)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(std::to_string(i));
    return *this;
}
QUERY& QUERY::operator << (const int32_t i)
{
    params_.push_back(std::to_string(i));
    return *this;
}
#ifndef _LP64
#if !defined(_WIN32) && !defined(__MACH__)
QUERY& QUERY::operator << (const time_t&i)
{
    if (params.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params.push_back(std::to_string((int32_t)i));
    return *this;
}
#endif
#endif
// QUERY& QUERY::operator << (const Rational& i)
// {
//     if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
//     params_.push_back(i.toString());
//     return *this;
// }
QUERY& QUERY::operator << (const uint32_t i)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(std::to_string(i));
    return *this;
}
QUERY& QUERY::operator << (const uint64_t i)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(std::to_string(i));
    return *this;
}
QUERY& QUERY::operator << (const int64_t i)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(std::to_string(i));

    return *this;
}
#if defined(_WIN32) || defined(__MACH__) && !defined __IOS__
QUERY& QUERY::operator << (const long i)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(std::to_string((int64_t)i));
    return *this;
}
#endif


QUERY& QUERY::operator << (const real d)
{
    if (params_.size()>=MAX_PARAMS_IN_SQL) throw CommonError("QUERY params > %d", MAX_PARAMS_IN_SQL);
    params_.push_back(std::to_string(d));
    return *this;
}

std::string QUERY::prepare() const
{
    std::string result;
    size_t qidx=0;
    for (unsigned int i=0; i<query_.size(); i++)
    {
        if (query_[i]!='?') result+=query_[i];
        else
        {
            if (params_.size()<=qidx)throw CommonError("query params size not equal ? "+query_+" "+_DMI());
            result+=params_[qidx];
            qidx++;
        }
    }
    return result;
}
