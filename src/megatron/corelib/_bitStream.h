#pragma once


#include <string>

struct BitsPosition
{
    size_t pos;
    size_t bitsRemainsInByte;
    BitsPosition(size_t p)
    {
        pos=p;
        bitsRemainsInByte=8;
    }
    void clear()
    {
        pos=0;
        bitsRemainsInByte=8;
    }
};


class inBitStream
{
    /// Note: byte operations possible if bits reminder = 0

private:
    BitsPosition m_pos;
    size_t m_size;
    const unsigned char *m_data;
    size_t m_BitsRemainsInByte;
public:
    ~inBitStream() {};
    inBitStream(const unsigned char *d, size_t size); // construct from an array
    inBitStream(const  char *d, size_t size); // construct from an array
    inBitStream(const  std::string&); // construct from string

    /// get full buffer size
    size_t size() const
    {
        return m_size;
    }
    /// get data pointer without fluence of read operations
    const unsigned char *data() const
    {
        return m_data;
    }
    /// get size of reminder
    size_t remains() const
    {
        return m_size - m_pos.pos;
    }
    /// check data is available for reading
    bool beforeEnd() const ;


public:
    /// unpack reminder. Byte operation!!
    std::string unpackRest();

    /// unpack  'size' bytes and put into  s. Throws if reminder size less than requested
    void unpack(std::string& s, size_t size);

    /// unpack  'size' bytes and put into  dst. 'success' indicate success of operation
    void unpack_nothrow(std::string& dst, size_t size, bool &success);

    /// byte op!
    unsigned int getTlvLength();

    /// byte op!
    unsigned char get_8();

    /// bit op!
    int get_bit();

    /// get reminder of ccurrent byte
    size_t paddingSize() const
    {
        return m_pos.bitsRemainsInByte;
    }
    /// get int from next 'bitcount' bits
    int get_bits(int bitcount);

    /// get 1 byte without exception.
    unsigned char get_8_nothrow(bool &success);

    /// get 2 bytes in Little Endian, throws on end of  buffer
    unsigned short get_16LE();

    /// get 2 bytes in Little Endian, nothrows
    unsigned short get_16LE_nothrow(bool &success);

    /// get 2 bytes in Big Endian, throws on end of  buffer
    unsigned short get_16BE();

    /// get 2 bytes in Little Endian, nothrows
    unsigned short get_16BE_nothrow(bool &success);

    /// get 4 bytes in Little Endian, throws on end of  buffer
    unsigned int get_32LE();

    /// get 4 bytes in Little Endian, nothrows
    unsigned int get_32LE_nothrow(bool &success);

    /// get 4 bytes in Big Endian, throws on end of  buffer
    unsigned int get_32BE();

    /// get 4 bytes in Big Endian, nothrows
    unsigned int get_32BE_nothrow(bool &success);

    /// get packed int, throws
    unsigned long get_PN();

    /// get packed int, nothrows
    unsigned long get_PN_nothrow(bool &success);

    /// get string, prefixed by len - packed number, throws
    std::string get_PSTR();

    /// get string, prefixed by len - packed number, throws
    std::string get_PSTR_nothrow(bool &success);

    /// buffer IO operations
    inBitStream& operator>>(int64_t&);
    inBitStream& operator>>(uint64_t&);
    inBitStream& operator>>(int32_t&);
    inBitStream& operator>>(uint32_t&);
    inBitStream& operator>>(int16_t&);
    inBitStream& operator>>(uint16_t&);
    inBitStream& operator>>(int8_t&);
    inBitStream& operator>>(uint8_t&);
    inBitStream& operator>>(std::string&);
    inBitStream& operator>>(bool&);
private:
    /// close operator for double
    inBitStream& operator>>(double&);
public:
};

class outBitStream
{
    /// Note: byte operations possible if bits reminder = 0
private:
    outBitStream(const outBitStream&);             // Not defined to prevent usage
    outBitStream& operator=(const outBitStream&);  // Not defined to prevent usage
public:
    unsigned char *buffer;
    size_t bufsize;
    BitsPosition m_pos;

    outBitStream();
    ~outBitStream();

    /// pack string
    outBitStream& pack(const std::string& s);

    /// pack buffer
    outBitStream& pack(const char * s, size_t len);

    /// pack buffer
    outBitStream& pack(const unsigned char * s, size_t len);

    const unsigned char *data()const
    {
        return buffer;
    }

    unsigned char *const_data()const
    {
        return buffer;
    }

    /// get bits reminder in current byte
    size_t paddingSize() const
    {
        return m_pos.bitsRemainsInByte;
    }
    /// get reminder as string, byte align required
    std::string asString() const;

    size_t size() const;

    void adjust(size_t n);
    void clear();
    outBitStream& putTLVLength(unsigned long);
    outBitStream& put_8(unsigned char);
    outBitStream& put_bits(unsigned int, int bitcout);

    /// put ops for little/big endian
    outBitStream& put_16LE(unsigned short);
    outBitStream& put_16BE(unsigned short);
    outBitStream& put_32LE(unsigned int);
    outBitStream& put_32BE(unsigned int);

    /// put packed num
    outBitStream& put_PN(unsigned long);

    /// put string, prefixed by packed num
    outBitStream& put_PSTR(const std::string &);

    /// put ops
    outBitStream& operator<<(const char*);
    outBitStream& operator<<(int64_t);
    outBitStream& operator<<(uint64_t);
    outBitStream& operator<<(int32_t);
    outBitStream& operator<<(uint32_t);
    outBitStream& operator<<(int16_t);
    outBitStream& operator<<(uint16_t);
    outBitStream& operator<<(int8_t);
    outBitStream& operator<<(uint8_t);
    outBitStream& operator<<(const std::string&);
    outBitStream& operator<<(const bool&);
private:
    /// close operator<< for double
    outBitStream& operator<<(const double&);
public:
private:

    /// internal methods
    outBitStream& put_8$(uint8_t c);
    outBitStream& put_8$(int8_t c);

    outBitStream& put_16LE$(uint16_t l);
    outBitStream& put_16BE$(uint16_t l);
    outBitStream& put_16LE$(int16_t l);
    outBitStream& put_16BE$(int16_t l);

    outBitStream& put_32LE$(uint32_t  l);
    outBitStream& put_32BE$(uint32_t l);

    outBitStream& put_32LE$(int32_t  l);
    outBitStream& put_32BE$(int32_t l);

    outBitStream& Pack$(const std::string& s );
    outBitStream& put_PN$(unsigned long N);


};

