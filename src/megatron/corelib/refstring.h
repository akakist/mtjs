#pragma once

#include "REF.h"
#include <string>
#include <inttypes.h>
// #include "commonError.h"
#include <string.h>

/// refbuffer - string replacement with recount
struct refbuffer: public Refcountable
{
private:
public:
    std::string container;

    // std::string asString();
    // refbuffer():buffer(NULL),size_(0),capacity(0) {}
    // ~refbuffer()
    // {
    //     if(buffer)
    //         ::free(buffer);
    // }
    // uint8_t * alloc(size_t sz)
    // {
    //     if(size_>=sz && buffer)
    //         return buffer;
    //     if(buffer)
    //         free(buffer);
    //     buffer=(uint8_t*)malloc(sz+0x20);
    //     size_=sz+10;
    //     return buffer;
    // }
};
REF_getter<refbuffer> toRef(std::string&& s);
REF_getter<refbuffer> toRef(const uint8_t* buf, int size);


