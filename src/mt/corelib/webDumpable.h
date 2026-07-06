#pragma once
#include <string>
#include <stdint.h>
/// base class to make object webdumpable on http page

class WebDumpable
{
private:
    WebDumpable(const WebDumpable&); // protect from usage
    WebDumpable& operator=(const WebDumpable&); // protect from usage

public:
    virtual std::string wname()=0;
    virtual std::string wdump()=0;
    virtual ~WebDumpable();
    WebDumpable();
    WebDumpable* getWebDumpablePtr()
    {
        return this;
    }
    std::string getWebDumpableLink(const std::string& visibleName);
    std::string getWebDumpableLink(const int64_t& vname);

};
