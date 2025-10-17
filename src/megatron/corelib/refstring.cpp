#include "refstring.h"
#include "commonError.h"
REF_getter<refbuffer> toRef(std::string&& s)
{

    if(s.size())
    {
        REF_getter<refbuffer> r=new refbuffer;
        r->container=std::move(s);
        return r;
    }
    REF_getter<refbuffer> r=new refbuffer;
    return r;
}
REF_getter<refbuffer> toRef(const uint8_t* buf, int size)
{
    if(size)
    {
        REF_getter<refbuffer> r=new refbuffer;
        r->container=std::string((char*)buf,size);
        return r;
    }
    REF_getter<refbuffer> r=new refbuffer;
    return r;
}
// std::string refbuffer::asString()
// {
//     if(size_ && buffer)
//         return std::string((char*)buffer,size_);
//     else return "";
// }
