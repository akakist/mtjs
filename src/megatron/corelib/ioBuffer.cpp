#include <stdlib.h>
#include <stdio.h>
#include "IUtils.h"
#include "ioBuffer.h"
#include "mutexInspector.h"
#include "commonError.h"



inBuffer::inBuffer(const unsigned char* d, size_t siz) :  out_pos(0), m_size(siz), m_data(d) { }
inBuffer::inBuffer(const char* d, size_t siz) :  out_pos(0), m_size(siz), m_data(( unsigned char*)d) { }

inBuffer::inBuffer(const std::string& s) :  out_pos(0), m_size(s.size()), m_data(( unsigned char*)s.data()) { }
inBuffer::inBuffer(const REF_getter<refbuffer>& s) :  out_pos(0), m_size(s->container.size()), m_data(( unsigned char*)s->container.data()) { }

unsigned char inBuffer::get_8()
{
    if (out_pos + 1 > m_size)
    {
        throw CommonError("inBuffer::get_8: noenough "+_DMI());
    }
    unsigned char c= m_data[out_pos++];
    return c;
}

outBuffer& outBuffer::pack(const std::string& s)
{
    XTRY;
    buffer->container+=s;
    // adjust(s.size());
    // memcpy(&buffer->buffer[cur_pos],s.data(),s.size());
    // cur_pos+=s.size();
    XPASS;
    return *this;
}
outBuffer& outBuffer::pack(const char * s, size_t len)
{
    XTRY;
    buffer->container.append(s,len);
    // adjust(len);
    // memcpy(&buffer->buffer[cur_pos],s,len);
    // cur_pos+=len;
    XPASS;
    return *this;
}
outBuffer& outBuffer::pack(const void * s, size_t len)
{
    XTRY;
    // adjust(len);
    buffer->container.append((char*)s,len);
    // memcpy(&buffer->buffer[cur_pos],s,len);
    // cur_pos+=len;
    XPASS;
    return *this;
}

void inBuffer::unpack(void* buf, size_t sz)
{
    if (sz > m_size-out_pos)
    {
        throw CommonError("unpack error not enough %s",_DMI().c_str());
    }
    memcpy(buf,&m_data[out_pos],sz);
    out_pos += sz;

}
void inBuffer::unpack_nothrow(std::string& s, size_t size, bool &success)
{
    XTRY;
    if (size > m_size-out_pos)
    {
        success=false;
        return;
    }
    s=std::string((char*)&m_data[out_pos],size);
    out_pos += size;
    success=true;
    XPASS;
}
std::string inBuffer::unpackRest()
{
    XTRY;
    std::string s;
    unpack(s,remains());
    return s;
    XPASS;
}

void inBuffer::unpack(std::string& s, int64_t size)
{
    XTRY;
    bool success;
    unpack_nothrow(s, static_cast<size_t>(size), success);
    if (!success) throw CommonError("inBuffer unpack:  buffer size to small sz=%ld remains=%d %s",size,remains(),_DMI().c_str());
    XPASS;
}
void outBuffer::clear()
{
    buffer->container.clear();
    // cur_pos=0;
}
REF_getter<refbuffer> outBuffer::asString() const
{

    return buffer;
    // buffer->size_=cur_pos;
    // buffer->buffer=(uint8_t*)realloc(buffer->buffer,buffer->size_+0x20);
    // if(!buffer->buffer)
    //     throw std::runtime_error("alloc error");
    // buffer->capacity=buffer->size_+0x20;
    // return buffer;
}
outBuffer::outBuffer(): buffer(new refbuffer)//:buffer(nullptr), bufsize(0),cur_pos(0)
{
    // construct();
}
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

outBuffer::~outBuffer()
{

}
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

outBuffer& outBuffer::put_8(unsigned char c)
{
    XTRY;
    // adjust(1);
    buffer->container+=(char)c;
    // buffer->buffer[cur_pos++]=c;
    XPASS;
    return *this;

}
outBuffer& outBuffer::put_PN(uint64_t N)
{
    return put_PN$(N);
}
std::string inBuffer::get_PSTR()
{
    XTRY;
    uint64_t sz=get_PN();
    std::string r;
    unpack(r,sz);
    return r;
    XPASS;
}
std::string inBuffer::get_PSTR_nothrow(bool &success)
{
    std::string container;
    XTRY;

    uint64_t sz=get_PN_nothrow(success);
    if (!success) return "";
    if (remains()<sz)
    {
        success=false;
        return "";
    }
    for (size_t i = 0; i < sz && beforeEnd(); i++)
    {
        unsigned char c=get_8_nothrow(success);
        if (!success) return "";
        container += std::string((char *)&c, 1);
    }
    XPASS;
    return container;
}
outBuffer& outBuffer::put_PSTR(const std::string & c)
{
    XTRY;
    put_PN$((unsigned long)c.size());
    Pack$(c);
    XPASS;
    return *this;
}
#ifdef __MACH__
#endif

inBuffer& inBuffer::operator>>(double& p)
{


    XTRY;
    std::string r;
    *this>>r;
    p=atof(r.c_str());
    XPASS;
    return *this;
}
inBuffer& inBuffer::operator>>(float& p)
{


    XTRY;
    std::string r;
    *this>>r;
    p= static_cast<float>(atof(r.c_str()));
    XPASS;
    return *this;
}

inBuffer& inBuffer::operator>>(std::string&p)
{
    XTRY;
    p=get_PSTR();
    XPASS;
    return *this;

}
inBuffer& inBuffer::operator>>(bool& p)
{
    XTRY;
    p= static_cast<bool>(get_PN());
    XPASS;
    return *this;

}


outBuffer& outBuffer::operator<<(const double &c)
{

    XTRY;
    auto s=std::to_string(c);
    *this<<s;
    XPASS;
    return *this;
}
outBuffer& outBuffer::operator<<(const float &c)
{

    XTRY;
    auto s=std::to_string(c);
    *this<<s;
    XPASS;
    return *this;
}
outBuffer& outBuffer::operator<<(const std::string& c)
{
    XTRY;
    put_PSTR(c);
    XPASS;
    return *this;
}
outBuffer& outBuffer::operator<<(const char* c)
{
    XTRY;
    put_PSTR(c);
    XPASS;
    return *this;
}
outBuffer& outBuffer::operator<<(const bool &c)
{
    XTRY;
    put_PN(static_cast<const uint64_t>(c));
    XPASS;
    return *this;
}

size_t outBuffer::size() const
{
    return buffer->container.size();
}

outBuffer& outBuffer::put_PN$(const uint64_t &N)
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

outBuffer& outBuffer::Pack$(const std::string& s )
{
    buffer->container+=s;
    // memcpy(&buffer->buffer[cur_pos],s.data(),s.size());
    // cur_pos+=(int)s.size();
    return *this;
}

size_t inBuffer::size() const
{
    return m_size;
}
const unsigned char *inBuffer::data()
{
    return m_data;
}
size_t inBuffer::remains() const
{
    return m_size - out_pos;
}
bool inBuffer::beforeEnd() const
{
    return (out_pos < m_size);
}
const unsigned char *outBuffer::data()const
{
    return (unsigned char *)buffer->container.data();
}
unsigned char *outBuffer::const_data()const
{
    return (unsigned char *)buffer->container.data();
}

outBuffer & operator<< (outBuffer& b,const REF_getter<refbuffer> &s)
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
inBuffer & operator>> (inBuffer& b,  REF_getter<refbuffer> &s)
{

    auto size= static_cast<size_t>(b.get_PN());
    s=new refbuffer;
    b.unpack(s->container,size);
    // if(size)
    // {
    //     s->buffer=(uint8_t*)malloc(size+0x20);
    //     if(!s->buffer) throw CommonError("memory alloc error %d",size);
    //     s->size_=size;
    //     s->capacity=size+0x20;
    //     b.unpack(s->buffer,s->size_);
    // }
    return b;
}
