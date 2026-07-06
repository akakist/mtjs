#include <stdlib.h>
#include <stdio.h>
#include "IUtils.h"
#include "_bitStream.h"
#include <map>
#include <string>
#include "commonError.h"
#include <bitset>
#include "mutexInspector.h"

inBitStream::inBitStream(const unsigned char* d, size_t siz) :  m_pos(0), m_size(siz), m_data(d),m_BitsRemainsInByte(8) { }
inBitStream::inBitStream(const char* d, size_t siz) :  m_pos(0), m_size(siz), m_data(( unsigned char*)d),m_BitsRemainsInByte(8) { }

inBitStream::inBitStream(const std::string& s) :  m_pos(0), m_size(s.size()), m_data(( unsigned char*)s.data()),m_BitsRemainsInByte(8) { }

outBitStream& outBitStream::pack(const std::string& s)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    adjust(s.size());
    memcpy(&buffer[m_pos.pos],s.data(),s.size());
    m_pos.pos+=s.size();
    return *this;
}

outBitStream& outBitStream::pack(const char * s, size_t len)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    adjust(len);
    memcpy(&buffer[m_pos.pos],s,len);
    m_pos.pos+=len;
    return *this;
}
outBitStream& outBitStream::pack(const unsigned char * s, size_t len)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    adjust(len);
    memcpy(&buffer[m_pos.pos],s,len);
    m_pos.pos+=len;
    return *this;
}
void inBitStream::unpack_nothrow(std::string& s, size_t size, bool &success)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    if (size > m_size-m_pos.pos)
    {
        success=false;
        return;
    }
    s=std::string((char*)&m_data[m_pos.pos],size);
    m_pos.pos += size;
    success=true;
}
std::string inBitStream::unpackRest()
{
    std::string s;
    unpack(s,remains());
    return s;
}

void inBitStream::unpack(std::string& s, size_t size)
{
    bool success;
    unpack_nothrow(s,size,success);
    if (!success) throw CommonError("inBitStream unpack: not enough buffer %s",_DMI().c_str());
}
void outBitStream::clear()
{
    m_pos.clear();
}



std::string outBitStream::asString() const
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder %d",m_pos.bitsRemainsInByte);

    return std::string((char*)buffer,m_pos.pos);
}
outBitStream::outBitStream():buffer(NULL), bufsize(0),m_pos(0)
{
    buffer=(unsigned char*)malloc(256);
    if (buffer==NULL) throw CommonError("alloc error");
    bufsize=256;
    m_pos.clear();
}
outBitStream::~outBitStream()
{
    if (buffer)
    {
        free(buffer);
        buffer=NULL;
    }

}
void outBitStream::adjust(size_t n)
{

    if (m_pos.pos+n>=bufsize)
    {
        size_t nnew=m_pos.pos+n+1024;
        buffer=(unsigned char*)realloc(buffer,nnew);
        if (!buffer) throw CommonError("alloc error sz=%d",nnew);
        bufsize=nnew;
    }

}
outBitStream& outBitStream::putTLVLength(unsigned long len)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder %d",m_pos.bitsRemainsInByte);

    try
    {
        if(len<128)
        {
            put_bits(0,1);
            put_bits(len,7);
            return *this;
        }
        else
        {
            put_bits(1,1);
            if(len<0x10000)
            {
                put_bits(2,7);
                put_bits(len,16);
            }
            else if(len<0x1000000)
            {
                put_bits(3,7);
                put_bits(len,24);
            }
            else
            {
                put_bits(4,7);
                put_bits(len,32);
            }
        }

    }
    catch(CommonError &e)
    {
        e.append("->outBitStream::putTLVLength");
        throw;
    }

    return *this;
}

outBitStream& outBitStream::put_8(unsigned char c)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder %d",m_pos.bitsRemainsInByte);
    adjust(1);
    buffer[m_pos.pos++]=c;
    return *this;

}
outBitStream& outBitStream::put_16LE(unsigned short l)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder %d",m_pos.bitsRemainsInByte);
    adjust(2);
    buffer[m_pos.pos++]=(l & 0xFF);
    buffer[m_pos.pos++]=((l>>8) & 0xFF);
    return *this;
}
outBitStream& outBitStream::put_16BE(unsigned short l)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder %d",m_pos.bitsRemainsInByte);
    adjust(2);
    buffer[m_pos.pos++]=((l>>8) & 0xFF);
    buffer[m_pos.pos++]=(l & 0xFF);
    return *this;
}
outBitStream& outBitStream::put_32LE(unsigned int l)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder %d",m_pos.bitsRemainsInByte);
    adjust(4);
    buffer[m_pos.pos++]=(l & 0xFF);
    buffer[m_pos.pos++]=((l >> 8) & 0xFF);
    buffer[m_pos.pos++]=((l >> 16) & 0xFF);
    buffer[m_pos.pos++]=((l >> 24) & 0xFF);
    return *this;

}
outBitStream& outBitStream::put_32BE(unsigned int l)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder ");
    adjust(4);
    buffer[m_pos.pos++]=((l >> 24) & 0xFF);
    buffer[m_pos.pos++]=((l >> 16) & 0xFF);
    buffer[m_pos.pos++]=((l >> 8) & 0xFF);
    buffer[m_pos.pos++]=(l & 0xFF);
    return *this;
}
unsigned char inBitStream::get_8()
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    if (m_pos.pos + 1 > m_size)
    {
        throw CommonError("inBitStream::get_8: noenough");
    }
    unsigned char c= m_data[m_pos.pos++];
    return c;
}
unsigned char inBitStream::get_8_nothrow(bool &success)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    if (m_pos.pos + 1 > m_size)
    {
        success=false;
        return 0;
    }
    success=true;
    unsigned char c= m_data[m_pos.pos++];
    return c;
}
unsigned short inBitStream::get_16LE()
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    if (m_pos.pos + 2 > m_size) throw CommonError("get_16LE: noenough");
    unsigned short l = ((unsigned short)m_data[m_pos.pos++]);
    l+= ((unsigned short)m_data[m_pos.pos++] << 8);
    return l;

}
unsigned short inBitStream::get_16LE_nothrow(bool &success)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    if (m_pos.pos + 2 > m_size)
    {
        success=false;
        return 0;
    }
    success=true;
    unsigned short l = ((unsigned short)m_data[m_pos.pos++]);
    l+= ((unsigned short)m_data[m_pos.pos++] << 8);
    return l;
}
unsigned short inBitStream::get_16BE()
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");

    if (m_pos.pos + 2 > m_size) throw CommonError("get_16BE: noenough");
    unsigned short  l = ((unsigned short)m_data[m_pos.pos++] << 8);
    l+= ((unsigned short)m_data[m_pos.pos++]);
    return l;
}
unsigned int inBitStream::get_32LE()
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    if (m_pos.pos + 4 > m_size)  throw CommonError("get_32LE: noenough");

    unsigned int   l = ((unsigned int)m_data[m_pos.pos++]);
    l+= ((unsigned int)m_data[m_pos.pos++] << 8);
    l+= ((unsigned int)m_data[m_pos.pos++] << 16);
    l+= ((unsigned int)m_data[m_pos.pos++] << 24);
    return l;

}
unsigned int inBitStream::get_32BE()
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");

    if (m_pos.pos + 4 > m_size)  throw CommonError("get_32BE: noenough");

    unsigned int
    l = ((unsigned int)m_data[m_pos.pos++] << 24);
    l+= ((unsigned int)m_data[m_pos.pos++] << 16);
    l+= ((unsigned int)m_data[m_pos.pos++] << 8);
    l+= ((unsigned int)m_data[m_pos.pos++]);
    return l;
}
unsigned long inBitStream::get_PN()
{

    unsigned long NN=0;
    while (1)
    {
        unsigned char c=get_8();
        if (c<0x80)
        {
            NN<<=7;
            NN|=c;
            return NN;
        }
        else
        {
            NN<<=7;
            NN|=c&0x7F;

        }
    }
}
unsigned long inBitStream::get_PN_nothrow(bool &success)
{
    unsigned long NN=0;
    while (1)
    {
        unsigned char c=get_8_nothrow(success);
        if (!success)return 0;
        if (c<0x80)
        {
            NN<<=7;
            NN|=c;
            return NN;
        }
        else
        {
            NN<<=7;
            NN|=c&0x7F;

        }
    }
}
outBitStream& outBitStream::put_PN(unsigned long N)
{
    return put_PN$(N);
}
std::string inBitStream::get_PSTR()
{
    std::string container = "";
    unsigned long sz=get_PN();
    if (remains()<sz) throw CommonError("remains()<sz remains(%d) size(%d)",remains(),size());
    for (size_t i = 0; i < sz && beforeEnd(); i++)
    {
        unsigned char c=get_8();

        container += std::string((char *)&c, 1);
    }
    return container;
}
std::string inBitStream::get_PSTR_nothrow(bool &success)
{
    std::string container = "";

    unsigned long sz=get_PN_nothrow(success);
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
    return container;
}
outBitStream& outBitStream::put_PSTR(const std::string & c)
{
    adjust(5+c.size());
    put_PN$((unsigned long)c.size());
    Pack$(c);
    return *this;
}

outBitStream& outBitStream::operator<<(int8_t c)
{
    XTRY;
    put_PN(c);
    XPASS;
    return *this;
}
outBitStream& outBitStream::operator<<(uint8_t c)
{
    XTRY;
    put_PN(c);
    XPASS;
    return *this;
}
outBitStream& outBitStream::operator<<(int16_t c)
{
    XTRY;
    put_PN(c);
    XPASS;
    return *this;
}
outBitStream& outBitStream::operator<<(uint16_t c)
{
    XTRY;
    put_PN(c);
    XPASS;
    return *this;
}
outBitStream& outBitStream::operator<<(int32_t c)
{
    XTRY;
    put_PN(c);
    XPASS;
    return *this;
}
outBitStream& outBitStream::operator<<(uint32_t c)
{
    XTRY;
    put_PN(c);
    XPASS;
    return *this;
}
inBitStream& inBitStream::operator>>(int8_t& p)
{
    XTRY;
    p=get_PN();
    XPASS;
    return *this;
}
inBitStream& inBitStream::operator>>(uint8_t& p)
{
    XTRY;
    p=get_PN();
    XPASS;
    return *this;
}
inBitStream& inBitStream::operator>>(int16_t& p)
{
    XTRY;
    p=get_PN();
    XPASS;
    return *this;
}
inBitStream& inBitStream::operator>>(uint16_t& p)
{
    XTRY;
    p=get_PN();
    XPASS;
    return *this;
}
inBitStream& inBitStream::operator>>(int32_t& p)
{
    XTRY;
    p=get_PN();
    XPASS;
    return *this;
}
inBitStream& inBitStream::operator>>(uint32_t& p)
{
    XTRY;
    p=get_PN();
    XPASS;
    return *this;
}
inBitStream& inBitStream::operator>>(int64_t& p)
{
    XTRY;
    p=get_PN();
    XPASS;
    return *this;
}
inBitStream& inBitStream::operator>>(uint64_t& p)
{
    XTRY;
    p=get_PN();
    XPASS;
    return *this;
}
inBitStream& inBitStream::operator>>(std::string&p)
{
    p=get_PSTR();
    return *this;

}
inBitStream& inBitStream::operator>>(bool& p)
{
    p=get_PN();
    return *this;

}

outBitStream& outBitStream::operator<<(const std::string& c)
{
    put_PSTR(c);
    return *this;
}
outBitStream& outBitStream::operator<<(const char* c)
{
    put_PSTR(c);
    return *this;
}
outBitStream& outBitStream::operator<<(const bool& c)
{
    put_PN(c);
    return *this;
}

size_t outBitStream::size() const
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    return m_pos.pos;
}
inBitStream& inBitStream::operator>>(double& v)
{
    std::string s;
    (*this)>>s;
    sscanf(s.c_str(),"%lf",&v);
    return *this;
}
outBitStream& outBitStream::put_PN$(unsigned long N)
{
    adjust(10);
    int start=28;
    if (sizeof(unsigned long)==8)
        start=56;

    for (int i=start; i>=0; i-=7)
    {
        uint8_t c=(N>>i) & 0x7F;
        if (c || i==0)
        {
            for (; i>=0; i-=7)
            {
                if (i) c=(N>>i) & 0x7F;
                else c=N & 0x7F;
                if (i)
                    put_8$(uint8_t(c|0x80));
                else
                    put_8$(c);
            }
            return *this;
        }

    }

    return *this;
}

outBitStream& outBitStream::operator<<(int64_t N)
{
    adjust(10);
    int start=28;
    if (sizeof(int64_t)==8)
        start=56;

    for (int i=start; i>=0; i-=7)
    {
        uint8_t c=(N>>i) & 0x7F;
        if (c || i==0)
        {
            for (; i>=0; i-=7)
            {
                if (i) c=(N>>i) & 0x7F;
                else c=N & 0x7F;
                if (i)
                    put_8$(uint8_t(c|0x80));
                else
                    put_8$(c);
            }
            return *this;
        }

    }

    return *this;
}

outBitStream& outBitStream::operator<<(uint64_t N)
{
    adjust(10);
    int start=28;
    if (sizeof(N)==8)
        start=56;

    for (int i=start; i>=0; i-=7)
    {
        uint8_t c=(N>>i) & 0x7F;
        if (c || i==0)
        {
            for (; i>=0; i-=7)
            {
                if (i) c=(N>>i) & 0x7F;
                else c=N & 0x7F;
                if (i)
                    put_8$(uint8_t(c|0x80));
                else
                    put_8$(c);
            }
            return *this;
        }

    }
    return *this;
}

outBitStream& outBitStream::put_32LE$(unsigned int l)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    buffer[m_pos.pos++]=(l & 0xFF);
    buffer[m_pos.pos++]=((l >> 8) & 0xFF);
    buffer[m_pos.pos++]=((l >> 16) & 0xFF);
    buffer[m_pos.pos++]=((l >> 24) & 0xFF);
    return *this;
}
outBitStream& outBitStream::put_32BE$(unsigned int l)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    buffer[m_pos.pos++]=((l >> 24) & 0xFF);
    buffer[m_pos.pos++]=((l >> 16) & 0xFF);
    buffer[m_pos.pos++]=((l >> 8) & 0xFF);
    buffer[m_pos.pos++]=(l & 0xFF);
    return *this;
}

outBitStream& outBitStream::put_16LE$(unsigned short l)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    buffer[m_pos.pos++]=(unsigned char)(l & 0xFF);
    buffer[m_pos.pos++]=((l>>8) & 0xFF);
    return *this;
}

outBitStream& outBitStream::put_16BE$(unsigned short l)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    buffer[m_pos.pos++]=((l>>8) & 0xFF);
    buffer[m_pos.pos++]=(l & 0xFF);
    return *this;
}
outBitStream& outBitStream::put_8$(unsigned char c)
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    buffer[m_pos.pos++]=c;
    return *this;
}
outBitStream& outBitStream::Pack$(const std::string& s )
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    memcpy(&buffer[m_pos.pos],s.data(),s.size());
    m_pos.pos+=(int)s.size();
    return *this;
}

outBitStream& outBitStream::put_bits(unsigned int val, int bitcount)
{
    if(!m_pos.bitsRemainsInByte) throw CommonError("boundary error on BitStream remainder");
    adjust(bitcount/8);
    for(int i=bitcount-1; i>=0; i--)
    {
        int bit=(val>>i)&1;
        buffer[m_pos.pos]<<=1;
        buffer[m_pos.pos]|=bit;
        m_pos.bitsRemainsInByte--;
        if(!m_pos.bitsRemainsInByte)
        {
            m_pos.pos++;
            m_pos.bitsRemainsInByte=8;
        }
    }
    return *this;
}
bool inBitStream::beforeEnd() const
{
    if(m_pos.bitsRemainsInByte!=8) throw CommonError("boundary error on BitStream remainder");
    return (m_pos.pos < m_size);
}

unsigned int inBitStream::getTlvLength()
{
    try
    {
        unsigned int b=get_bit();
        if(b==0)
        {
            return get_bits(7);
        }
        else
        {
            int llen=get_bits(7);
            if(llen>4) throw CommonError("if(llen>4)");
            return get_bits(llen*8);
        }

    }
    catch(CommonError &e)
    {
        e.append("->getTlvLength");
        throw;
    }
}
int inBitStream::get_bit()
{
    int bit=(m_data[m_pos.pos] >> (m_pos.bitsRemainsInByte-1)) &1;
    m_pos.bitsRemainsInByte--;
    if(!m_pos.bitsRemainsInByte)
    {
        m_pos.pos++;
        m_pos.bitsRemainsInByte=8;
    }
    return bit;
}
int inBitStream::get_bits(int bitcount)
{
    int val=0;
    for(int i=bitcount-1; i>=0; i--)
    {
        int bit=get_bit();;
        val<<=1;
        val|=bit;
    }
    return val;
}
