#include "webDumpable.h"
#include "IUtils.h"
#ifdef WEBDUMP
WebDumpable::~WebDumpable()
{
    iUtils->removeWebDumpableHandler(this);
}
WebDumpable::WebDumpable()
{
    iUtils->setWebDumpableHandler(this);
}
std::string WebDumpable::getWebDumpableLink(const std::string& vname)
{
    WebDumpable* p=this;
    std::string s((char*)&p,sizeof(p));
    return "<a href='/webdump?p="+iUtils->bin2hex(s)+"'>"+vname+"</a>";
}
std::string WebDumpable::getWebDumpableLink(const int64_t& vname)
{
    WebDumpable* p=this;
    std::string s((char*)&p,sizeof(p));
    return "<a href='/webdump?p="+iUtils->bin2hex(s)+"'>"+std::to_string(vname)+"</a>";
}
#endif
