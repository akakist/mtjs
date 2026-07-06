#pragma once
#include <map>
#include <string>
/// base class for interfaces with casting ability

class UnknownBase
{
public:
    const std::string classname;
    virtual ~UnknownBase();
    virtual void deinit()=0;
    UnknownBase(const std::string& nm);

};

