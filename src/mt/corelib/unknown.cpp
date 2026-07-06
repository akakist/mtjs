#include "unknown.h"
#include <string>
UnknownBase::~UnknownBase()
{
}

UnknownBase::UnknownBase(const std::string &nm)
    : classname(nm)
{
}
