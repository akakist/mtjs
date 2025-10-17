#include <stdlib.h>
#include <stdio.h>
#include "IUtils.h"
#include "obuf.h"
#include "mutexInspector.h"
#include "commonError.h"




oBuf& oBuf::pack(const std::string& s)
{
    XTRY;
    if(cur_pos+s.size() > bufsize) throw std::bad_alloc();
    // buffer->container+=s;
    // adjust(s.size());
    memcpy(&buffer[cur_pos],s.data(),s.size());
    cur_pos+=s.size();
    XPASS;
    return *this;
}
oBuf& oBuf::pack(const char * s, size_t len)
{
    XTRY;
    if(cur_pos+len > bufsize) throw std::bad_alloc();
    // buffer->container.append(s,len)
    // adjust(len);
    memcpy(&buffer[cur_pos],s,len);
    cur_pos+=len;
    XPASS;
    return *this;
}
oBuf& oBuf::pack(const void * s, size_t len)
{
    XTRY;
    if(cur_pos+len > bufsize) throw std::bad_alloc();
    // adjust(len);
    // buffer->container.append((char*)s,len);
    memcpy(&buffer[cur_pos],s,len);
    cur_pos+=len;
    XPASS;
    return *this;
}

void oBuf::clear()
{
    // buffer->container.clear();
    cur_pos=0;
}
REF_getter<refbuffer> oBuf::asString() const
{

    REF_getter<refbuffer> r=new refbuffer;
    r->container=std::string(buffer,cur_pos);
    return r;
    // buffer->size_=cur_pos;
    // buffer->buffer=(uint8_t*)realloc(buffer->buffer,buffer->size_+0x20);
    // if(!buffer->buffer)
    //     throw std::runtime_error("alloc error");
    // buffer->capacity=buffer->size_+0x20;
    // return buffer;
}
// oBuf::oBuf(): buffer(new refbuffer)//:buffer(nullptr), bufsize(0),cur_pos(0)
// {
//     // construct();
// }
#define SPLICE__ 0x400
// void outBuffer::construct()
// {
//     ;

//     // buffer->buffer=(unsigned char*)malloc(SPLICE__);
//     // buffer->capacity=SPLICE__;
//     // if (buffer==nullptr) throw CommonError("alloc error");
//     // bufsize=SPLICE__;
//     // cur_pos=0;

// }

// void outBuffer::adjust(size_t n)
// {

//     XTRY;
//     if (cur_pos+n>=bufsize)
//     {
//         size_t nnew=cur_pos+n+0x10000;
//         buffer->buffer=(unsigned char*)realloc(buffer->buffer,nnew);
//         if (!buffer->buffer) throw CommonError("alloc error sz=%d",nnew);
//         bufsize=nnew;
//     }

//     XPASS;
// }

oBuf& oBuf::put_8(unsigned char c)
{
    XTRY;
    // adjust(1);
    if(cur_pos+1>bufsize)
        throw std::bad_alloc();
    buffer[cur_pos++]=(char)c;
    // buffer->buffer[cur_pos++]=c;
    XPASS;
    return *this;

}
oBuf& oBuf::put_PN(uint64_t N)
{
    return put_PN$(N);
}
oBuf& oBuf::put_PSTR(const std::string & c)
{
    XTRY;
    put_PN$((unsigned long)c.size());
    Pack$(c);
    XPASS;
    return *this;
}
#ifdef __MACH__
#endif



oBuf& oBuf::operator<<(const double &c)
{

    XTRY;
    auto s=std::to_string(c);
    *this<<s;
    XPASS;
    return *this;
}
oBuf& oBuf::operator<<(const float &c)
{

    XTRY;
    auto s=std::to_string(c);
    *this<<s;
    XPASS;
    return *this;
}
oBuf& oBuf::operator<<(const std::string& c)
{
    XTRY;
    put_PSTR(c);
    XPASS;
    return *this;
}
oBuf& oBuf::operator<<(const char* c)
{
    XTRY;
    put_PSTR(c);
    XPASS;
    return *this;
}
oBuf& oBuf::operator<<(const bool &c)
{
    XTRY;
    put_PN(static_cast<const uint64_t>(c));
    XPASS;
    return *this;
}

size_t oBuf::size() const
{
    return cur_pos;
}

oBuf& oBuf::put_PN$(const uint64_t &N)
{
    XTRY;
    // adjust(10);
    int start=63;

    /// loop to skip spaces
    for (int i=start; i>=0; i-=7)
    {
        unsigned char c=(N>>i) & 0x7F;
        if (c || i==0)
        {
            for (; i>=0; i-=7)
            {
                if (i)
                {
                    c=(N>>i) & 0x7F;
                    put_8$(c|0x80);
                }
                else
                {
                    c=N & 0x7F;
                    put_8$(c);
                }
            }
            return *this;
        }

    }
    XPASS;
    return *this;
}

oBuf& oBuf::Pack$(const std::string& s )
{
    // buffer->container+=s;
    memcpy(&buffer[cur_pos],s.data(),s.size());
    cur_pos+=s.size();
    return *this;
}

const unsigned char *oBuf::data()const
{
    return (unsigned char *)buffer;
}
unsigned char *oBuf::const_data()const
{
    return (unsigned char *)buffer;
}

oBuf & operator<< (oBuf& b,const REF_getter<refbuffer> &s)
{

    bool packed=false;
    if(s.valid())
    {
        // if(s->buffer)
        {
            b.put_PN(s->container.size());
            b.pack(s->container);
            packed=true;
        }
    }
    if(!packed)
        b.put_PN(0);

    return b;
}
