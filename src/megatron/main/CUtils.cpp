#define _FILE_OFFSET_BITS 64
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#include "commonError.h"
#include <sys/stat.h>
#include <stdlib.h>
#include "CUtils.h"
#include <dirent.h>
#if !defined __ANDROID__ && !defined __FreeBSD__
#include <sys/timeb.h>
#endif
#include <unistd.h>
#ifndef _WIN32
#include <netdb.h>
#endif
#include "mutexInspector.h"
#ifndef _MSC_VER
#endif
#ifdef _WIN32
#else
#include <dlfcn.h>
#endif

#include "st_malloc.h"
#include <fcntl.h>
#include "st_FILE.h"
#ifdef _WIN32
#include <lib/regex/regex.h>
#else
#include <regex.h>
#endif
#include <zlib.h>
#include <version_mega.h>
#include "megatron_config.h"
#include "CInstance.h"
#include "threadNameCtl.h"
#include "genum.hpp"
#include "resplit.h"
const char* CUtils::genum_name(int n)
{

    return __gen_string123(n);

}

std::map<std::string,std::string> CUtils::loadStringMapFromBuffer(const std::string &bod, const char *linedelim)
{
    std::string section;
    std::vector<std::string> v=splitString(linedelim,bod);
    std::map<std::string,std::string>m;
    for (size_t i=0; i<v.size(); i++)
    {
        std::string theString=v[i];
        bool empty=true;

        for (size_t kk=0; kk<theString.size(); kk++)
        {
            if (!isspace(theString[kk]))
            {
                empty=false;
                break;
            }
        }
        while (theString.size())
        {
            if (isspace(theString[0])) theString=theString.substr(1,theString.size()-1);
            else break;
        }

        if (empty) continue;
        if (theString.size())
        {
            if (theString[0]=='#')continue;
        }
        if (theString[0]=='%')
        {
            section=theString.substr(1,theString.size()-1);
            continue;

        }

        size_t pz=theString.find("=",0);
        if (pz==std::string::npos)
        {
            throw CommonError("--Error: syntax error in line %d ",i);
        }
        std::string key;
        if (section.size())key=section+"_"+theString.substr(0,pz);
        else key=theString.substr(0,pz);

        std::string val=theString.substr(pz+1,theString.size()-pz-1);
        m[key]=val;
    }
    return m;
}
std::map<std::string,std::string> CUtils::loadStringMapFromFile(const std::string &fn)
{
    XTRY;
    std::map<std::string,std::string>m;
    std::string bod;
    std::string section;
    long res=load_file_from_disk(bod,fn);
    if(res<0) return m;
    std::vector<std::string> v=splitString("\r\n",bod);
    for (size_t i=0; i<v.size(); i++)
    {
        std::string theString=v[i];
        bool empty=true;

        for (size_t kk=0; kk<theString.size(); kk++)
        {
            if (!isspace(theString[kk]))
            {
                empty=false;
                break;
            }
        }
        while (theString.size())
        {
            if (isspace(theString[0])) theString=theString.substr(1,theString.size()-1);
            else break;
        }

        if (empty) continue;
        if (theString.size())
        {
            if (theString[0]=='#')continue;
        }
        if (theString[0]=='%')
        {
            section=theString.substr(1,theString.size()-1);
            continue;

        }

        size_t pz=theString.find("=",0);
        if (pz==std::string::npos)
        {
            throw CommonError("--Error: syntax error in file %s line %d ",fn.c_str(),i);
        }
        std::string key;
        if (section.size())key=section+"_"+theString.substr(0,pz);
        else key=theString.substr(0,pz);

        std::string val=theString.substr(pz+1,theString.size()-pz-1);
        m[key]=val;
    }
    return m;
    XPASS;

}
std::deque<std::string> CUtils::splitStringDQ(const char *seps, const std::string & src)
{
    if(src.empty())
        return {};
    std::string r=(std::string)"["+seps+"]";
    auto d=resplitDQ(src,std::regex(r));
    std::deque<std::string> res;
    for(auto& z:d)
    {
        if(z.size())
            res.push_back(std::move(z));
    }
    return res;
}
std::set < std::string> CUtils::splitStringSET(const char *seps, const std::string & src)
{
    if(src.empty())
        return {};
    std::string r=(std::string)"["+seps+"]";
    auto d=resplitSET(src,std::regex(r));
    std::set < std::string> res;
    for(auto& z:d)
    {
        if(z.size())
            res.insert(std::move(z));
    }
    return res;

}
std::vector<std::string> CUtils::splitString(const char *seps, const std::string & src)
{
    if(src.empty())
        return {};

    std::string r=(std::string)"["+seps+"]";
    auto d=resplit(src,std::regex(r));
    std::vector < std::string> res;
    for(auto& z:d)
    {
        if(z.size())
            res.push_back(std::move(z));
    }
    return res;

}
std::string CUtils::join(const char *pattern, const std::set < std::string> &st)
{

    std::string ret;
    std::vector<std::string> arr;
    for (auto& j:st)
    {
        arr.push_back(j);
    }
    return join(pattern,arr);
}
std::string CUtils::join(const char* pattern,const std::deque<std::string>& arr)
{

    std::string ret;
    if (arr.size() > 1)
    {
        unsigned int i;
        for (i = 0; i < arr.size() - 1; i++)
            ret += arr[i] + pattern;
        ret += arr[arr.size() - 1];
    }
    else if (arr.size() == 1)
    {
        ret += arr[0];
    }
    return ret;

}
std::string CUtils::join(const char* pattern,const std::vector<std::string>& arr)
{

    std::string ret;
    if (arr.size() > 1)
    {
        unsigned int i;
        for (i = 0; i < arr.size() - 1; i++)
            ret += arr[i] + pattern;
        ret += arr[arr.size() - 1];
    }
    else if (arr.size() == 1)
    {
        ret += arr[0];
    }
    return ret;

}
std::string CUtils::trim(const std::string &s)
{
    std::deque<char> d;
    for (unsigned k=0; k<s.size(); k++)
    {
        d.push_back(s[k]);
    }
    while (!d.empty())
    {
        if (isspace(*d.begin())) d.pop_front();
        else break;
    }
    while (!d.empty())
    {
        if (isspace(*d.rbegin())) d.pop_back();
        else break;
    }
    std::string ret;
    for (unsigned i=0; i<d.size(); i++)
        ret+=d[i];
    return ret;
}


static unsigned char l_table[] =
{
    /**
    lowercase table
    */
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd,
    0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d,
    0x3e, 0x3f, 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61,
    0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d,
    0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x90, 0x83, 0x82, 0x83, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89, 0x9a, 0x8b, 0x9c, 0x9d, 0x9e, 0x9f, 0x90, 0x91,
    0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d,
    0x9e, 0x9f, 0xa0, 0xa2, 0xa2, 0xbc, 0xa4, 0xb4, 0xa6, 0xa7, 0xb8, 0xa9,
    0xba, 0xab, 0xac, 0xad, 0xae, 0xbf, 0xb0, 0xb1, 0xb3, 0xb3, 0xb4, 0xb5,
    0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbe, 0xbe, 0xbf, 0xe0, 0xe1,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed,
    0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
    0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,
    0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1,
    0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd,
    0xfe, 0xff
};

static unsigned char u_table[] =
{
    /**
    uppercase table
    */
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd,
    0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d,
    0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
    0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x41,
    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d,
    0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x81, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x80, 0x91,
    0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x8a, 0x9b, 0x8c, 0x8d,
    0x8e, 0x8f, 0xa0, 0xa1, 0xa1, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
    0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb2, 0xa5, 0xb5,
    0xb6, 0xb7, 0xa8, 0xb9, 0xaa, 0xbb, 0xa3, 0xbd, 0xbd, 0xaf, 0xc0, 0xc1,
    0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd,
    0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
    0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1,
    0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd,
    0xde, 0xdf
};


std::string CUtils::strupper(const std::string &s)
{

    std::string a;
    for (unsigned int i=0; i<s.length(); i++)
    {
        unsigned char z=s[i];
        a+=u_table[z];
    }
    return a;
}

std::string CUtils::strlower(const std::string & s)
{

    /**
    convert std::string to lowercase
    */
    std::string a;
    for (unsigned int i = 0; i < s.length(); i++)
    {
        unsigned char z = s[i];
        a += l_table[z];
    }
    return a;

}
static char    six2pr[64] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

static unsigned char pr2six[256];


/** --- function HTUU_encode -----------------------------------------------
 *
 *   Encode a single line of binary data to a standard format that
 *   uses only printing ASCII characters (but takes up 33% more bytes).
 *
 *    Entry    bufin    points to a buffer of bytes.  If nbytes is not
 *                      a multiple of three, then the byte just beyond
 *                      the last byte in the buffer must be 0.
 *             nbytes   is the number of bytes in that buffer.
 *                      This cannot be more than 48.
 *             bufcoded points to an output buffer.  Be sure that this
 *                      can hold at least 1 + (4*nbytes)/3 characters.
 *
 *    Exit     bufcoded contains the coded line.  The first 4*nbytes/3 bytes
 *                      contain printing ASCII characters representing
 *                      those binary bytes. This may include one or
 *                      two '=' characters used as padding at the end.
 *                      The last byte is a zero byte.
 *             Returns the number of ASCII characters in "bufcoded".
 */
static int
Ns_HtuuEncode(unsigned char *bufin, unsigned int nbytes, char * bufcoded)
{

    /** ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) six2pr[c]

    char  *outptr = bufcoded;
    unsigned int    i;

    for (i = 0; i < nbytes; i += 3)
    {
        /** c1 */
        *(outptr++) = ENC(*bufin >> 2);
        /** c2 */
        *(outptr++) = ENC(((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017));
        /** c3 */
        *(outptr++) = ENC(((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03));
        /** c4 */
        *(outptr++) = ENC(bufin[2] & 077);

        bufin += 3;
    }

    /**
     * If nbytes was not a multiple of 3, then we have encoded too many
     * characters.  Adjust appropriately.
     */
    if (i == nbytes + 1)
    {
        /** There were only 2 bytes in that last group */
        outptr[-1] = '=';
    }
    else if (i == nbytes + 2)
    {
        /** There was only 1 byte in that last group */
        outptr[-1] = '=';
        outptr[-2] = '=';
    }
    *outptr = '\0';
    return (outptr - bufcoded);
}

/** --- function HTUU_decode ------------------------------------------------
 *
 *  Decode an ASCII-encoded buffer back to its original binary form.
 *
 *    Entry    bufcoded    points to a uuencoded string.  It is
 *                         terminated by any character not in
 *                         the printable character table six2pr, but
 *                         leading whitespace is stripped.
 *             bufplain    points to the output buffer; must be big
 *                         enough to hold the decoded std::string (generally
 *                         shorter than the encoded string) plus
 *                         as many as two extra bytes used during
 *                         the decoding process.
 *             outbufsize  is the maximum number of bytes that
 *                         can fit in bufplain.
 *
 *    Exit     Returns the number of binary bytes decoded.
 *             bufplain    contains these bytes.
 */
static int
Ns_HtuuDecode(char * bufcoded, unsigned char * bufplain, int outbufsize)
{

    /** single character decode */
#define DEC(c) pr2six[(int)c]
#define MAXVAL 63

    static int      first = 1;

    int             nbytesdecoded, j;
    char  *bufin = bufcoded;
    unsigned char *bufout = bufplain;
    int    nprbytes;

    /**
     * If this is the first call, initialize the mapping table. This code
     * should work even on non-ASCII machines.
     */
    if (first)
    {
        first = 0;
        for (j = 0; j < 256; j++)
            pr2six[j] = MAXVAL + 1;

        for (j = 0; j < 64; j++)
            pr2six[(int) six2pr[j]] = (unsigned char) j;
    }

    /** Strip leading whitespace. */

    while (*bufcoded == ' ' || *bufcoded == '\t')
        bufcoded++;

    /**
     * Figure out how many characters are in the input buffer. If this would
     * decode into more bytes than would fit into the output buffer, adjust
     * the number of input bytes downwards.
     */
    bufin = bufcoded;
    while (pr2six[(int) *(bufin++)] <= MAXVAL) {};
    nprbytes = bufin - bufcoded - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;
    if (nbytesdecoded > outbufsize)
    {
        nprbytes = (outbufsize * 4) / 3;
    }
    bufin = bufcoded;

    while (nprbytes > 0)
    {
        *(bufout++) = (unsigned char) (DEC(*bufin) << 2 | DEC(bufin[1]) >> 4);
        *(bufout++) = (unsigned char) (DEC(bufin[1]) << 4 |
                                       DEC(bufin[2]) >> 2);
        *(bufout++) = (unsigned char) (DEC(bufin[2]) << 6 | DEC(bufin[3]));
        bufin += 4;
        nprbytes -= 4;
    }

    if (nprbytes & 03)
    {
        if (pr2six[(int) bufin[-2]] > MAXVAL)
        {
            nbytesdecoded -= 2;
        }
        else
        {
            nbytesdecoded -= 1;
        }
    }
    return (nbytesdecoded);
}


std::string CUtils::Base64Encode(const std::string &str)
{
    size_t len=str.size()*2;
    char *out=(char*)malloc(len+10);
    if(!out)
        throw std::runtime_error("alloc error");
    ::memset(out,0,len+2);
    int outlen=Ns_HtuuEncode((unsigned char*)str.data(),str.size(),out);
    std::string ret(out,outlen);
    free(out);
    return ret;
}
std::string CUtils::Base64Decode(const std::string &str)
{

    if(str.size()==0) return "";
    size_t len=str.size();
    char *out=(char*)malloc(len+10);
    if(!out)
        throw CommonError("!out size %d",str.size());
    ::memset(out,0,len+2);
    int outlen=Ns_HtuuDecode((char*)str.data(),(unsigned char*)out,len);
    std::string ret(out,outlen);
    free(out);
    return ret;
}

std::string CUtils::hex2bin(const std::string &s)
{
    std::string out="";
    char *p=(char*)s.data();
    size_t sz=s.size();
    for (size_t i=0; i<sz; i+=2)
    {
        char ss[3]= {0};
        ::memcpy(ss,&p[i],2);
        unsigned char c=(unsigned char)strtol(ss,NULL,16);
        out+=std::string((char*)&c,1);
    }
    return out;
}

std::string CUtils::bin2hex(const std::string & in)
{
    std::string out = "";
    XTRY;
    const unsigned char *p = (unsigned char *)in.data();
    for (unsigned int i = 0; i < in.size(); i++)
    {
        XTRY;
        char s[40];
        ::snprintf(s, sizeof(s) - 1, "%02x", p[i]);
        out += s;
        XPASS;
    }
    XPASS;
    return out;
}
long CUtils::load_file_from_disk(std::string & res, const std::string & fn)
{
    XTRY;

#ifdef _MSC_VER
    struct _stat  st;
#else
    struct stat  st;
#endif
#ifdef _MSC_VER
    if (_stat(fn.c_str(), &st))
#else
    if (stat(fn.c_str(), &st))
#endif

    {
        logErr2("stat: errno %d",errno);
        return -1;
    }
    st_FILE f(fn, "rb");
    {
        st_malloc b(st.st_size);
        if ((off_t)fread(b.buf,1,st.st_size,f.f)!=st.st_size)
        {
            throw CommonError("if(fread(b,1,st.st_size,f)!=st.st_size)  %s",fn.c_str());
        }
        std::string al((char *)b.buf, st.st_size);
        res=al;
        return st.st_size;
    }
    return -1;
    XPASS;

}
std::string CUtils::load_file(const std::string & fn)
{
    XTRY;
    std::string res;
    long r=load_file_from_disk(res,fn);
    if(r<0)
    {
        throw CommonError("cannot stat file %s",fn.c_str());
    }
    return res;
    XPASS;
}
std::string CUtils::load_file_no_throw(const std::string & fn)
{
    XTRY;
    std::string res;

    try {
        int r=load_file_from_disk(res,fn);

        if(r<0)
        {
            logErr2("cannot stat file %s",fn.c_str());
            return "";
        }
        return res;
    } catch(...)
    {
        return "";
    }

    XPASS;
}
std::string CUtils::unescapeURL(const std::string & s)
{
    std::string a;
    for (unsigned int i = 0; i < s.size(); i++)
    {
        if (s[i] == '%')
        {
            if (i + 2 > s.size())
            {
                break;
            }
            int c1 = (int)u_table[(int)s[i + 1]];
            int c2 = (int)u_table[(int)s[i + 2]];
            i += 2;
            if (c1 > 70 || c2 > 70)
            {
                continue;
            }
            if (c1 < 48 || c2 < 48)
            {
                continue;
            }
            if (c1 >= 48 && c1 <= 57)
            {
                c1 -= 48;
            }
            if (c2 >= 48 && c2 <= 57)
            {
                c2 -= 48;
            }
            if (c1 >= 65 && c1 <= 70)
            {
                c1 -= 55;
            }
            if (c2 >= 65 && c2 <= 70)
            {
                c2 -= 55;
            }
            a += (char) c1 *16 + c2;
        }
        else
        {
            if (s[i] == '+')
            {
                a += " ";
            }
            else
            {
                a += s[i];
            }
        }
    }
    return a;
}

std::string CUtils::extractFileExt(const std::string & f)
{


    std::string::size_type  zz;
    zz = f.rfind(".", f.size());
    if (zz != std::string::npos)
    {
        return f.substr(zz + 1, f.size() - zz - 1);
    }

    return "";
}
std::string CUtils::extractFileName(const std::string & f)
{


    std::string::size_type  zz;
    zz = f.rfind("/", f.size());
    if (zz != std::string::npos)
    {
        return f.substr(zz + 1, f.size() - zz - 1);
    }

    return f;
}

std::string CUtils::get_param_string(std::deque<std::string> &tokens, const std::string& name)
{
    if(tokens.size()==0) throw CommonError("%s skipped",name.c_str());
    std::string ret=tokens[0];
    tokens.pop_front();
    return ret;
}
int CUtils::get_param_int(std::deque<std::string> &tokens, const std::string& name)
{
    if(tokens.size()==0) throw CommonError("%s skipped",name.c_str());
    std::string ret=tokens[0];
    tokens.pop_front();
    return atoi(ret.c_str());
}
int64_t CUtils::get_param_int64_t(std::deque<std::string> &tokens, const std::string& name)
{
    if(tokens.size()==0) throw CommonError("%s skipped",name.c_str());
    std::string ret=tokens[0];
    tokens.pop_front();
#ifndef _MSC_VER
    return atoll(ret.c_str());
#else
    return atoi(ret.c_str());
#endif
}


int64_t CUtils::getNow()
{

    uint64_t now;
#if !defined(_WIN32)
    timespec tstmp;
    if(clock_gettime(CLOCK_REALTIME,&tstmp))
    {
        throw CommonError("clock_gettime: errno %d",errno);
    }
    uint64_t _1=tstmp.tv_sec;
    _1*=1000000;
    uint64_t _2=tstmp.tv_nsec;
    _2/=1000;

    now=_1;
    now+=_2;
    return now;
#else
    timeb tb;
    ftime(&tb);
    uint64_t add=tb.millitm;
    add*=1000;
    now=tb.time;
    now*=1000000;
    now+=add;
    Integer v;
    v.set(now);
    return v;
#endif

}
std::string CUtils::getPercent(const real& numerator, const real& denumerator)
{
    char s[100];
    real pr=100;
    snprintf(s,sizeof(s),"%0.f%%",float((numerator*pr)/denumerator));
    return s;
}

int CUtils::getHostByName(const char* hostname,unsigned int &out)
{
    {
        R_LOCK(hostnames.lk);
        auto i=hostnames.cache.find(hostname);
        if(i!=hostnames.cache.end())
        {
            out=i->second;
            return 0;
        }
    }
    struct hostent *hostEnt;
    DBG(logErr2("gethostbyname %s",hostname));
    if ((hostEnt = gethostbyname(hostname)) == NULL || hostEnt->h_addrtype != AF_INET)
    {
        return -1;
    }
    out=*((unsigned int *)(hostEnt->h_addr));
    {
        W_LOCK(hostnames.lk);
        hostnames.cache[hostname]=out;
    }
    return 0;

}

std::string CUtils::dump(const std::set<msockaddr_in> &s)
{
    std::vector<std::string> v;
    for(auto& i:s)
    {
        v.push_back(i.dump());
    }
    return join(",",v);
}
std::string CUtils::dump(const std::deque<msockaddr_in> &s)
{
    std::vector<std::string> v;
    for(auto& i:s)
    {
        v.push_back(i.dump());
    }
    return join(",",v);
}
std::string CUtils::dump(const std::vector<msockaddr_in> &s)
{
    std::vector<std::string> v;
    v.reserve(s.size());
    for(auto& i: s)
    {
        v.push_back(i.dump());
    }
    return join(",",v);
}
std::string CUtils::dump(const std::set<std::pair<msockaddr_in,std::string> > &s)
{
    std::vector<std::string> v;
    v.reserve(s.size());
    for(auto& i: s)
    {
        v.push_back(i.first.dump()+":"+bin2hex(i.second));
    }
    return join(",",v);
}
std::string CUtils::dump(const std::map<SERVICE_id,std::set<msockaddr_in> > &s)
{
    std::vector<std::string> v;
    v.reserve(s.size());
    for(auto& i:s)
    {
        v.push_back(iUtils->serviceName(i.first)+"#"+dump(i.second));
    }
    return join(",",v);
}

std::string CUtils::dump(const std::map<msockaddr_in,std::set<SERVICE_id> > &s)
{
    std::vector<std::string> v;
    v.reserve(s.size());
    for(auto& i:s)
    {
        std::vector<std::string> vs;
        for(auto& j:i.second)
        {
            vs.push_back(iUtils->serviceName(j));
        }
        v.push_back(i.first.dump()+"#"+join(",",vs));
    }
    return join(";",v);
}


int64_t CUtils::calcFileSize(const std::string & fn)
{
    int _fd=::open(fn.c_str(),O_RDONLY);
    if(_fd==-1)
    {

        return -1;
    }
    st_FD fd(_fd);
#if defined __MACH__ || defined __FreeBSD__
    int64_t size=lseek(fd.fd,0,SEEK_END);
#else
    int64_t size=lseek64(fd.fd,0,SEEK_END);
#endif
    if(size==-1)
    {
        logErr2("lseek64: %s errno %d",errno,fn.c_str());
        return -1;
    }
    return size;
}

bool is_file_exists(const std::string &pathname)
{

    struct stat st;
    if(stat(pathname.c_str(),&st)) {
        return false;
    }
    return true;
}
int CUtils::checkPath(const std::string & _pathname)
{
    struct stat st;
    if(!stat(_pathname.c_str(),&st)) return 0;
    std::string pathname=_pathname;
#ifdef _WIN32
    std::vector <std::string> ar=splitString("\\/",pathname);;
#else
    std::vector <std::string> ar=splitString("/",pathname);
#endif

#ifdef _WIN32
#define split_char '\\';
#else
#define split_char '/';
#endif

    std::string parts;
    if((pathname.substr(0,1)=="/") || (pathname.substr(0,1)=="\\")) parts+=split_char;
    for(unsigned i=0; i<ar.size(); i++)
    {
        std::string par=parts+ar[i];
        parts+=ar[i]+split_char;
        if((!is_file_exists(parts))&&(!is_file_exists(par)))
        {
            int res=
#ifdef _WIN32
                mkdir(parts.c_str());
#else
                mkdir(parts.c_str(),0755);
#endif
            if(res) return -1;
        }
    }
    return 0;
}
std::string CUtils::findExecutable(const std::string& _fn)
{
    std::string fn=_fn;;
#ifdef _WIN32
    fn+=".exe";
#endif
    std::vector<std::string> searchPath;
    searchPath.push_back("./");
#ifdef _WIN32
    searchPath.push_back("");
#endif
#ifdef _WIN32
    char *pf1=getenv("ProgramFiles");
    char* pf2=getenv("ProgramFiles(x86)");
    if(pf1)
    {
    }
    if(pf2)
    {
    }
#else
    searchPath.push_back("/usr/bin/");
    searchPath.push_back("/usr/local/bin/");
#endif
    for(size_t i=0; i<searchPath.size(); i++)
    {
        std::string fileName=searchPath[i]+fn;
        struct stat st;
        if(!stat(fileName.c_str(),&st))
            return fileName;
    }
    return "";
}
void CUtils::rxfind(std::vector < rxfind_data > &res, const char *regexp, const char *buffer)
{

    XTRY;
    int err;
    regex_t re;
    err = regcomp(&re, regexp, REG_EXTENDED);
    regmatch_t pm[10];
    ::memset(pm, 0, sizeof(regmatch_t));
    if (err) {
        char erbuf[111];
        regerror(err, &re, erbuf, sizeof(erbuf));
        logErr2("regex: %s",erbuf);
        throw CommonError((std::string)"regex "+regexp+": "+erbuf);
        return;
    }
    size_t lastpos = 0;
    while (!regexec(&re, buffer + lastpos, 10, pm, 0)) {
        XTRY;
        const char *s = buffer + lastpos;
        size_t lpos=lastpos;
        for (int i = 0; i < 10; i++) {
            XTRY;
            if (pm[i].rm_so != -1) {
                XTRY;

                size_t matchlen = pm[i].rm_eo - pm[i].rm_so;

                XTRY;
                std::string fs(&s[pm[i].rm_so], matchlen);
                XTRY;
                rxfind_data fd;
                fd.str = fs;
                fd.so = pm[i].rm_so + lastpos;
                fd.eo = pm[i].rm_eo + lastpos;
                res.push_back(fd);
                XPASS;
                XPASS;
                XPASS;
                lpos=pm[i].rm_eo;
            } else break;
            XPASS;
        }

        lastpos += lpos;
        XPASS;
    }
    regfree(&re);
    XPASS;
}

std::string CUtils::str_replace(const std::string &s,const std::string &sr,const std::string &src)
{
    std::vector<rxfind_data> r;
    XTRY;
    rxfind(r,s.c_str(),src.c_str());
    XPASS;
    int lpz=0;
    std::string a;
    for(unsigned int i=0; i<r.size(); i++) {
        int eo=r[i].so-lpz;
        a+=src.substr(lpz,eo);
        a+=sr;
        lpz=r[i].eo;
    }
    int eo=src.size()-lpz;
    a+=src.substr(lpz,eo);
    return a;
}
std::string char2hex( char dec )
{
    char dig1 = (dec&0xF0)>>4;
    char dig2 = (dec&0x0F);
    if ( 0<= dig1 && dig1<= 9) dig1+=48;    //0,48inascii
    if (10<= dig1 && dig1<=15) dig1+=97-10; //a,97inascii
    if ( 0<= dig2 && dig2<= 9) dig2+=48;
    if (10<= dig2 && dig2<=15) dig2+=97-10;

    std::string r;
    r.append( &dig1, 1);
    r.append( &dig2, 1);
    return r;
}

std::string CUtils::uriEncode(const std::string &c)
{

    std::string escaped="";
    int max = c.length();
    for(int i=0; i<max; i++)
    {
        if ( (48 <= c[i] && c[i] <= 57) ||//0-9
                (65 <= c[i] && c[i] <= 90) ||//abc...xyz
                (97 <= c[i] && c[i] <= 122) || //ABC...XYZ
                (c[i]=='~' || c[i]=='!' || c[i]=='*' || c[i]=='(' || c[i]==')' || c[i]=='\'')
           )
        {
            escaped.append( &c[i], 1);
        }
        else
        {
            escaped.append("%");
            escaped.append( char2hex(c[i]) );//converts char 255 to string "ff"
        }
    }
    return escaped;
}

std::string CUtils::uriDecode(const std::string &SRC)
{
    std::string ret;
    char ch;
    int  ii;
    for (size_t i=0; i<SRC.length(); i++) {
        if (int(SRC[i])==37) {
            sscanf(SRC.substr(i+1,2).c_str(), "%x", &ii);
            ch=static_cast<char>(ii);
            ret+=ch;
            i=i+2;
        } else {
            ret+=SRC[i];
        }
    }
    return (ret);
}
std::string CUtils::replace_vals(std::map<std::string,std::string> &p, const std::string &src)
{
    if(!p.size())return src;
    std::string s;
    auto it=p.begin();
    s+=it->first;
    it++;
    /*SER: ERROR zdes mozhet byt esli map pustoj*/
    for(; it!=p.end(); it++) {
        s+="|"+(*it).first;
    }
    std::vector<IUtils::rxfind_data> r;
    std::string a;
    iUtils->rxfind(r,s.c_str(),src.c_str());
    int lpz=0;
    for(unsigned int i=0; i<r.size(); i++) {
        a+=src.substr(lpz,r[i].so-lpz);
        a+=p[r[i].str];
        lpz=r[i].eo;
    }
    a+=src.substr(static_cast<unsigned int>(lpz), src.size() - lpz);
    return a;
}




SOCKET_id CUtils::getNewSocketId()
{
    return sockIdGen.get();
}



Ifaces::Base* CUtils::queryIface(const SERVICE_id &id)
{
    if(m_isTerminating) throw CommonError("terminating");
    {
        M_LOCK(local.ifaces.lk);
        auto i=local.ifaces.container.find(id);
        if(i==local.ifaces.container.end())
        {
            {
                MutexUnlocker aaa(local.ifaces.lk);
                std::string pn;
                {
                    M_LOCK(local.pluginInfo);
                    auto iii=local.pluginInfo.ifaces.find(id);
                    if(iii!=local.pluginInfo.ifaces.end())
                    {
                        pn=iii->second;
                    }
                    else
                    {
                        for(auto z: local.pluginInfo.ifaces)
                        {
                            logErr2("have iface: %s -> %s",iUtils->genum_name(z.first),z.second.c_str());
                        }
                        throw CommonError("iface %s not found",iUtils->genum_name(id));
                    }
                }
                registerPluginDLL(pn);
            }
            i=local.ifaces.container.find(id);
            if(i==local.ifaces.container.end())
            {
                throw CommonError("cannot find iface %s %s",iUtils->genum_name(id),_DMI().c_str());
            }
        }

        return i->second;
    }

}
std::string CUtils::expand_homedir(const std::string& base)
{
    char *home=getenv("HOME");
    std::string _base;
    {
        for(size_t i=0; i<base.size(); i++)
        {
            if(base[i]=='~')
            {
                if(home)
                {
                    _base+=home;
                }
            }
            else _base+=base[i];
        }
    }
    return _base;
}
std::string CUtils::gLogDir()
{
#ifdef __MOBILE__
    if(m_files_dir.size()==0)
        throw CommonError("gLogDir: if(m_files_dir.size()==0)");
    return m_files_dir;
#endif
    {
        std::string cachedir=expand_homedir(LOG_TARGET_DIR);
        iUtils->checkPath(cachedir);
        return cachedir;
    }
    return ".";
}

std::string CUtils::gCacheDir()
{
#ifdef __MOBILE__
    if(m_files_dir.size()==0)
        throw CommonError("gCacheDir: if(m_files_dir.size()==0)");
    return m_files_dir;
#endif
    {
        std::string cachedir=expand_homedir(CACHE_TARGET_DIR);
        iUtils->checkPath(cachedir);
        return cachedir;
    }
    return ".";
}
std::string CUtils::gConfigDir()
{
#ifdef __MOBILE__
    if(m_files_dir.size()==0)
        throw CommonError("gConfigDir: if(m_files_dir.size()==0)");
    return m_files_dir;
#endif

    std::string cachedir=expand_homedir(CONFIG_TARGET_DIR);
    iUtils->checkPath(cachedir);
    return cachedir;
    return ".";
}
std::string CUtils::filesDir()
{
    return m_files_dir;
}
void CUtils::setFilesDir(const std::string& s)
{
    m_files_dir=s;
}


void CUtils::registerPluginDLL(const std::string& pn)
{
    if(m_isTerminating)return;

#ifdef _WIN32
    HMODULE h=LoadLibraryA(pn.c_str());
#else
    void *h=dlopen(pn.c_str(),RTLD_LAZY);
#endif
    if (!h)
    {
        printf(RED("cannot load plugin %s"),pn.c_str());
    }
    if (h)
    {
        {
            M_LOCK(registered_dlls);
            registered_dlls.registered_dlls.insert(h);
        }
#ifdef DEBUG
        const char *funcname="registerModuleDebug";
#else
        const char *funcname="registerModule";
#endif
#ifdef _WIN32
        REGOBJECT f=(REGOBJECT )GetProcAddress(h,funcname);
#else
        REGOBJECT f=(REGOBJECT)dlsym(h,funcname);
#endif
        if (f)
        {
            f(this,NULL);
        }
        else
        {
#ifdef _WIN32
            FreeLibrary(h);
#else
            dlclose(h);
#endif
            {
                M_LOCK(registered_dlls);
                registered_dlls.registered_dlls.erase(h);
            }

        }
    }
}
void CUtils::registerPlugingInfo(const char* pluginFileName, PluginType pt, const SERVICE_id& id, const char* name, const std::set<EVENT_id> &evts)
{
    if(m_isTerminating)return;

    // if(CONTAINER(version)>>8!=COREVERSION>>8)
    // {
    //     logErr2("if(version!=COREVERSION) in registerPlugingInfo");
    //     return;
    // }
    {
        M_LOCK(local.pluginInfo);
        switch (pt) {
        case IUtils::PLUGIN_TYPE_IFACE:
            DBG(printf(BLUE("added iface info %s %s -> %s"),iUtils->genum_name(id),name,pluginFileName));
            local.pluginInfo.ifaces[id]=pluginFileName;
            break;
        case IUtils::PLUGIN_TYPE_SERVICE:
            DBG(printf(BLUE("added service info %s %s -> %s"),iUtils->genum_name(id),name,pluginFileName));
            local.pluginInfo.services[id]=pluginFileName;
            for(auto& z:evts)
            {
                local.pluginInfo.events[z].insert(pluginFileName);
            }


            break;

        default:
            throw CommonError("invalid plugintype %d %s %d",pt, __FILE__,__LINE__);
            break;
        }
    }
    if(pt==IUtils::PLUGIN_TYPE_SERVICE)
    {
        W_LOCK(local.service_names.lk);
        local.service_names.id2name[id]=name;
        local.service_names.name2id[name]=id;
    }
}
void CUtils::registerService(const SERVICE_id& id, unknown_static_constructor cs, const std::string& literalName)
{
    if(m_isTerminating)return;

    DBG(printf(BLUE("registerService %s"),literalName.c_str()));
    // VERSION_id curv=COREVERSION;
    // if(CONTAINER(vid)>>8 != CONTAINER(curv)>>8)
    // {
    //     logErr2(RED("registerService: invalid service version %x current version %x name %s"),CONTAINER(vid), CONTAINER(curv), literalName.c_str());
    //     return;
    // }

    XTRY;
    {
        W_LOCK(local.service_constructors.lk);
        if(local.service_constructors.container.count(id))
        {
            DBG(printf(BLUE("Service '%s' already registered"),literalName.c_str()));
        }
        else
        {
            local.service_constructors.container.insert(std::make_pair(id,cs));
            DBG(printf(BLUE("Service '%s' registered"),literalName.c_str()));
        }

    }
    {
        W_LOCK(local.service_names.lk);
        local.service_names.id2name[id]=literalName;
        local.service_names.name2id[literalName]=id;
    }
    XPASS;
}

REF_getter<Event::Base> CUtils::unpackEvent(inBuffer&b)
{
    if(m_isTerminating) return nullptr;
    MUTEX_INSPECTOR;
    XTRY;
    EVENT_id id;
    b>>id;
    route_t r;
    b>>r;


    event_static_constructor esc=nullptr;

    {
        R_LOCK(local.event_constructors.lk);
        auto i=local.event_constructors.container.find(id);
        if(i!=local.event_constructors.container.end())
        {
            esc=i->second;
        }

    }
    if(esc==NULL)
    {
        MUTEX_INSPECTOR;
//        logErr2("1cannot find event unpacker for eventId %s try to load service %s", id.dump().c_str(),_DMI().c_str());
        {
            M_LOCK(local.pluginInfo);
            auto j=local.pluginInfo.events.find(id);
            if(j!=local.pluginInfo.events.end())
            {

                if(j->second.size())
                {
                    std::string pluginFileName=*j->second.begin();
                    M_UNLOCK(local.pluginInfo);
                    logErr2("load service %s",pluginFileName.c_str());
                    registerPluginDLL(pluginFileName);
                }
            }
            else
            {
                logErr2("event not binded to module %s",iUtils->genum_name(id));
            }
        }
        {

            R_LOCK(local.event_constructors.lk);
            auto i=local.event_constructors.container.find(id);
            if(i!=local.event_constructors.container.end())
            {
                esc=i->second;
            }
        }
    }
    {
        MUTEX_INSPECTOR;
        if(esc==nullptr)
            throw CommonError("Event constructor not found for %s",iUtils->genum_name(id));

        Event::Base *e=esc(r);
        {
            MUTEX_INSPECTOR;
            e->unpack(b);
        }
        return e;
    }
    return nullptr;
    XPASS;
}
void CUtils::packEvent(outBuffer &b, const REF_getter<Event::Base> &e)const
{
    XTRY;

    b<<e->id<<e->route;
    e->pack(b);
    XPASS;
}
std::string CUtils::serviceName(const SERVICE_id& id) const
{
    XTRY;
    R_LOCK(local.service_names.lk);
    {
        auto i=local.service_names.id2name.find(id);
        if(i!=local.service_names.id2name.end())
            return i->second;
    }
    return (std::string)"jc:"+iUtils->genum_name(id);
    XPASS;

}
SERVICE_id CUtils::serviceIdByName(const std::string& name)const
{
    MUTEX_INSPECTOR;
    XTRY;
    {
        R_LOCK(local.service_names.lk);
        auto i=local.service_names.name2id.find(name);
        if(i!=local.service_names.name2id.end())
            return i->second;
    }

    logErr2("serviceIdByName: NOT FOUND %s",name.c_str());
    throw CommonError ("Service name not found  %s %s",name.c_str(),_DMI().c_str());
    XPASS;
}



#ifdef WEBDUMP
void CUtils::setWebDumpableHandler(WebDumpable *h)
{
    M_LOCK(webDumping);
    webDumping.container.insert(h);
}
void CUtils::removeWebDumpableHandler(WebDumpable *h)
{
    M_LOCK(webDumping);
    webDumping.container.erase(h);
}
std::string CUtils::dumpWebDumpable(WebDumpable *h)
{
    M_LOCK(webDumping);
    auto i=webDumping.container.find(h);
    if(i!=webDumping.container.end())
    {
        return   "<h1>"+h->wname()+"</h1>\n"
                 +
                 "<p>\n<pre>" + h->wdump()+"\n</pre>";

    }
    return "unknown WebDumpable";
}
#endif

void CUtils::registerEvent(event_static_constructor ec)
{
    if(m_isTerminating)return;

    XTRY;
    route_t r;
    REF_getter<Event::Base> e=ec(r);
    if(!e.valid())
    {
        return;
    }
    {
        W_LOCK(local.event_constructors.lk);
        local.event_constructors.container.insert(std::make_pair(e->id,ec));
    }

    XPASS;

}
bool CUtils::isServiceRegistered(const SERVICE_id& svs)
{
    {
        R_LOCK(local.service_constructors.lk);
        if(local.service_constructors.container.count(svs))
            return true;
    }
    {
        M_LOCK(local.pluginInfo);
        if(local.pluginInfo.services.count(svs))
            return true;
    }
    return false;
}
IThreadNameController* CUtils::getIThreadNameController()
{
    static ThreadNameController threadCtl;
    return & threadCtl;
}
void CUtils::registerIface(const SERVICE_id& id, Ifaces::Base* p)
{

    M_LOCK(local.ifaces.lk);
    auto i=local.ifaces.container.find(id);
    if(i!=local.ifaces.container.end())
    {
        logErr2("register iface, already registered %s",iUtils->genum_name(id));
        return;
    }
    local.ifaces.container.insert(std::make_pair(id,p));
}
Utils_local *CUtils::getLocals()
{
    return &local;
}
std::set<IInstance *> CUtils::getInstances()
{
    if(m_isTerminating) throw CommonError("if(m_isTerminating)");
    R_LOCK (instances.lk);
    return instances.container;
}
void CUtils::registerInstance(IInstance *i)
{
    if(m_isTerminating)return;
    W_LOCK (instances.lk);
    instances.container.insert(i);
}
void CUtils::unregisterInstance(IInstance *i)
{
    delete i;
    W_LOCK (instances.lk);
    instances.container.erase(i);
}
IInstance* CUtils::createNewInstance(const std::string& nm)
{
    if(m_isTerminating) return NULL;
    IInstance *ins=new CInstance(this,nm);
    {
        W_LOCK (instances.lk);
        instances.container.insert(ins);
    }
    return ins;
}
void CUtils::setTerminate(int exit_flag)
{
    m_isTerminating=true;
    m_exit_code=exit_flag;
}
bool CUtils::isTerminating()
{
    return m_isTerminating;

}
CUtils::~CUtils()
{
    MUTEX_INSPECTOR;

    m_isTerminating=true;

    clearPollable();

    instances.clear();
    local.clear();
    m_addrInfos=NULL;
    registered_dlls.clear();
}

void CUtils::load_plugins_info(const std::set<std::string>& bases)
{
#ifndef _MSC_VER
    for (auto& I:bases)
    {
        std::string base=iUtils->expand_homedir(I);


        DIR * dir=opendir(base.c_str());
        if (!dir)
        {
            continue;
        }
        struct dirent* de;
        std::set<std::string> ss;
        while ((de=readdir(dir))!=NULL)
        {
            bool ok=false;
            ok=true;
            std::string n=de->d_name;
            if(n=="." || n=="..")
                continue;
            if (ok)
            {
                std::string pn;
                pn=(std::string)base+"/"+(std::string)de->d_name;

                ss.insert(pn);
                DBG(printf(BLUE("plugin %s"),pn.c_str()));
            }
        }
        for (auto& i:ss)
        {
            std::string pn=i;
            struct stat st;
            if (stat(pn.c_str(),&st))
            {
                printf(RED("cannot load plugin %s"),pn.c_str());
            }
            DBG(printf(BLUE("load plugin %s"),pn.c_str()));
            if (!stat(pn.c_str(),&st))
            {
#ifdef _WIN32
                HMODULE h=LoadLibraryA(pn.c_str());
#else
                void *h=dlopen(pn.c_str(),RTLD_LAZY);
#endif
                if (!h)
                {
                    printf(RED("cannot load plugin %s"),pn.c_str());
                }

                if (h)
                {
#ifdef DEBUG
                    const char *funcname="registerModuleDebug";
#else
                    const char *funcname="registerModule";
#endif
#ifdef _WIN32
                    REGOBJECT f=(REGOBJECT )GetProcAddress(h,funcname);
#else
                    REGOBJECT f=(REGOBJECT)dlsym(h,funcname);
#endif
                    if (f)
                    {
                        f(iUtils,i.c_str());
                    }
#ifdef _WIN32
                    FreeLibrary(h);
#else
                    dlclose(h);
#endif
                }
            }

        }
        closedir(dir);
    }
#endif

}


