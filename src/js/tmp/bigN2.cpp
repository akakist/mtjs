//#include <QCoreApplication>
#include <deque>
#include <string>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <cstdint>

static char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int chartoval(char c)
{
    switch(c)
    {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'A': return 10;
        case 'B': return 11;
        case 'C': return 12;
        case 'D': return 13;
        case 'E': return 14;
        case 'F': return 15;
        case 'G': return 16;
        case 'H': return 17;
        case 'I': return 18;
        case 'J': return 19;
        case 'K': return 20;
        case 'L': return 21;
        case 'M': return 22;
        case 'N': return 23;
        case 'O': return 24;
        case 'P': return 25;
        case 'Q': return 26;
        case 'R': return 27;
        case 'S': return 28;
        case 'T': return 29;
        case 'U': return 30;
        case 'V': return 31;
        case 'W': return 32;
        case 'X': return 33;
        case 'Y': return 34;
        case 'Z': return 35;    }
    throw std::runtime_error("invalid char in chartoval");
}
struct bigN
{
    std::deque<uint8_t> __v;
    int bitlen;
    int skip=0;
    bigN(){
        bitlen=0;
    }
    bigN(const uint64_t &n)
    {
        *this=bigN::from_uint64_t(n);
    }
    void reset()
    {
        __v.clear();
        bitlen=0;
        skip=0;
    }
    void setBit(int i, uint8_t v)
    {
        while(i>=bitlen)
        {
            push_back(0);
        }
        __v[i]=v;
    }
    uint8_t getBit(int i) const
    {
        if(i<0) return 0;
        if(i<bitlen)
            return __v[i];
        return 0;
    }
    static bigN from_uint64_t(uint64_t n)
    {
        bigN v;
        for(int i=0;i<64;i++)
        {
            if(((uint64_t(1)<<i)&n))
            {
                v.setBit(i,1);
            }
        }
        v.trim();
        return v;
    }
    void push_back(uint8_t v)
    {
        if(bitlen!=__v.size())
        {
            printf("111 %d %lu\n",bitlen,__v.size());
            throw std::runtime_error("111");

        }
        bitlen++;
        __v.push_back(v);
    }
    void push_front(uint8_t v)
    {
        if(bitlen!=__v.size())
        {
            printf("111 %d %lu\n",bitlen,__v.size());
            throw std::runtime_error("222");

        }
        bitlen++;
        __v.push_front(v);
    }
    void pop_back()
    {
        if(bitlen!=__v.size())
            throw std::runtime_error("333");
        bitlen--;
        __v.pop_back();
    }
    
    std::string dump() const
    {
        std::string s;
        for(auto& z: __v)
        {
            s+=z?"1":"0";
        }
        return s;
    }
    uint64_t to_uint64()
    {
        uint64_t r=0;
        for(int i=0;i<bitlen;i++)
        {
            r|=uint64_t(getBit(i))<<i;
        }

        return r;
    }
    private:
    void trim()
    {
        while( __v.size() && *__v.rbegin()==0)
            pop_back();
    }
    public:
    std::string to_string(int radix=10)
    {
        bigN _10=bigN::from_uint64_t(radix);
        bigN _0=bigN::from_uint64_t(0);
        _10.trim();
        _0.trim();
        bigN r=*this;
        r.trim();
        std::vector<bigN> reminders;
        std::string s;
        while(r!=_0)
        {
            auto res=div(r,_10);
            reminders.push_back(res.second);
            r=res.first;
            s=digits[res.second.to_uint64()]+s;
            r.trim();
        }
        return s;
    }
    static bigN from_string(const std::string & s, int radix=10)
    {
        bigN n;
        bigN _10=bigN::from_uint64_t(radix); 
        // _10.binarize(10);
        for(int i=0; i < s.size();i++)
        {
            if(!isdigit(s[i]) && !(s[i]>='A' && s[i]<='Z'))
            throw std::runtime_error("if(!isdigit(s[i]))");
            int c=chartoval(s[i]);
            bigN cc;
            cc=bigN::from_uint64_t(c);
            n=n*_10;
            n=n+cc;


        }
        n.trim();
        return n;

    }

    bool operator==(const bigN& b) const
    {
        return __v==b.__v;
    }
    bool operator!=(const bigN& b) const
    {
        return __v!=b.__v;
    }

    static bigN sub(const bigN& a, const bigN&b);
    static std::pair<bigN,bigN> div(const bigN& a, const bigN& _b);
    static bigN sum(const bigN& a, const bigN&b, int shiftLen=0);
    static bigN mul(const bigN& a, const bigN& b);
    bigN operator*(const bigN& b) const
    {
        return mul(*this,b);
    }
    bigN operator+(const bigN& b) const
    {
        return sum(*this,b);
    }
    bool greater(const bigN& a, const bigN& b) const;
    bool operator>(const bigN& b) const
    {
        return greater(*this,b);
    }


};

inline bigN bigN::sub(const bigN& a, const bigN&b)
{

    bigN r;
    int reminder=0;
    auto N=std::max(a.bitlen,b.bitlen);
    for(int i=0;i < N+1;i++)
    {
        int sum=0;
        if(reminder)
        {
            sum--;
            reminder=0;
        }
        if(i<a.bitlen)
        {
            sum+=a.__v[i];
        }
        if(i<b.bitlen)
        {
            sum-=b.__v[i];
        }
        switch (sum) {

        case 0:
            // r.push_back(0);
            break;
        case 1:
            r.setBit(i,1);
            break;
        case -1:
            r.setBit(i,1);
            reminder=1;
            break;
        case -2:
            // r.push_back(0);
            reminder=1;
            break;
        }
    }
    r.trim();
    return r;

}
bigN bigN::sum(const bigN& a, const bigN&b, int shiftLen)
{
    bigN r;
    int reminder=0;
    auto N=std::max(a.bitlen,b.bitlen+shiftLen);
    for(int i=0;i < N+1;i++)
    {
        int sum=0;
        if(reminder)
        {
            sum++;
            reminder=0;
        }
        if(i<a.bitlen)
        {
            sum+=a.getBit(i);
        }
        if(i<b.bitlen+shiftLen)
        {
            sum+=b.getBit(i-shiftLen);
        }
        switch (sum) {

        case 0:
            break;
        case 1:
            r.setBit(i,1);
            break;
        case 2:
            reminder=1;
            break;
        case 3:
            r.setBit(i,1);
            reminder=1;
            break;
        }
    }
    r.trim();
    return r;
}
bool bigN::greater(const bigN& a, const bigN& b) const
{
    int maxsize=std::max(a.bitlen,b.bitlen);
    for(int i=maxsize-1;i>=0;i--)
    {
        int8_t __a=0;
        int8_t __b=0;
        if(a.bitlen>i)
            __a=a.getBit(i);
        if(b.bitlen>i)
            __b=b.getBit(i);
        if(__a>__b)
            return true;
        if(__a==__b)
            continue;
        if(__a<__b)
            return false;

    }
    return false;
}
bigN bigN::mul(const bigN& a, const bigN& b)
{
//    std::cout<< "mul "<< a.dump() << " " << b.dump()<< "\n";
    bigN s;
    for(int i=0;i<b.bitlen;i++)
    {
        if(b.getBit(i))
        {
            s=sum(s,a, i);
        }
    }
    s.trim();
    return s;

}
inline std::pair<bigN,bigN> bigN::div(const bigN& a, const bigN& _b)
{
    bigN r;
    bigN aa=a;
    bigN b=_b;
    b.trim();

    aa.trim();
    bigN part;
    for(int i=aa.bitlen-1;i>=0;i--)
    {
        int8_t byte=aa.getBit(i);
        part.push_front(byte);
        part.trim();
        if(part==b)
        {
                r.setBit(i,1);
                part.reset();
        }
        else if(part>b)
        {
                r.setBit(i,1);
                part=sub(part,b);
                part.trim();
        }
    }
    r.trim();
    part.trim();
    return std::make_pair(r,part);
}


void test()
{
    for(int i=0;i<100000;i++)
    {
        bigN n;
        uint64_t r=uint64_t(rand())+uint64_t(rand())<<32;
        n=bigN::from_uint64_t(r);
        uint64_t n2=n.to_uint64();
        if(n2!=r)
            printf("error %lu\n",r);
    }
    for(int i=0;i<100;i++)
    {
        uint64_t r1=(uint64_t(rand())+uint64_t(rand())<<32)/10;
        uint64_t r2=(uint64_t(rand())+uint64_t(rand())<<32)/10;

        bigN n1=bigN::from_uint64_t(r1),n2=bigN::from_uint64_t(r2);
        auto s=n1+n2;
        auto res=s.to_uint64();
        if(r1+r2!=res)
            printf("sum wrong %lu - %ld\n",r1+r2,res);


    }
    for(int i=0;i<100;i++)
    {
        uint64_t r1=i;//rand();
        uint64_t r2=i+1;//rand();
        bigN n1=bigN::from_uint64_t(r1),n2=bigN::from_uint64_t(r2);
        auto n3=n1*n2;
        auto r3=n3.to_uint64();
        // n3.trim();
        if(r1*r2!=r3)
            std::cout<< "error mul1 "<< r1*r2 << " mul2 "<< r3 <<"\n";
    }
    for(int i=0;i< 1000;i++)
    {
        uint64_t r1=rand();
        uint64_t r2=rand();
        if(r1>r2)
        {
            bigN n1=bigN::from_uint64_t(r1),n2=bigN::from_uint64_t(r2);
            auto n3=bigN::sub(n1,n2);
            auto r3=n3.to_uint64();
            if(r1-r2!=r3)
                std::cout<< "sub1 "<< r1-r2 << " sub2 "<< r3 <<"\n";
        }


    }
    for(int i=0;i< 1000;i++)
    {
        uint64_t r1=rand();
        uint64_t r2=rand();
        if(r1>r2)
        {
            bigN 
            n1=bigN::from_uint64_t(r1),
            n2=bigN::from_uint64_t(r2);
            // n1.binarize(r1);
            // n2.binarize(r2);
            if(n1>n2)
            {
//                printf("ok greater\n");
            }
            else
            {
                printf("!ok greater\n");
            }
        }

    }

    for(int i=1;i< 1000;i++)
    {
        uint64_t r1=rand()%1000000;
        uint64_t r2=rand()%1000;
        if(r1>r2 && r2>0)
        {
            bigN n1=bigN::from_uint64_t(r1),
            n2=bigN::from_uint64_t(r2);
            // n1.binarize(r1);
            // n2.binarize(r2);
            auto n3=bigN::div(n1,n2);

            auto r3=n3.first.to_uint64();
            auto reminder=n3.second.to_uint64();
            if(r3!=r1/r2)
                std::cout<< "div1 r1="<<r1 <<" r2="<<r2<<" div(int)"<< r1/r2 << " div2 "<< r3 << " reminder "<< reminder<< " reminder2 "<<r1%r2 <<" \n";

        }

    }
//#ifdef KALL
    for(int i=1;i< 1000;i++)
    {
        uint64_t r1=rand();
        bigN n1,n2;
        n1=bigN::from_uint64_t(r1);
        auto s=n1.to_string();
        if(std::to_string(r1)!=s)
            std::cout<< "10num ="<<s<<" arith="<<r1<< " \n";


    }

    for(int i=100'000'000;i< 100'010'000;i++)
    {

        char s[200];
        snprintf(s,sizeof(s),"%d",i);
        bigN n=bigN::from_string(s);
        auto ss=n.to_string();
        if(ss!=s)
        {
            printf("s!=ss %s %s\n",s,ss.c_str());
        }

    }
//#endif

}

void factorial(int n)
{
    bigN res=bigN::from_uint64_t(1);
    for(int i=1;i<=n;i++)
    {
        bigN I=bigN::from_uint64_t(i);
        res=res*I;
//        printf("i %d\n",i);
    }
   std::string s=res.to_string(10);
   std::cout<< "factorial of "<<n<<"="<<s<<"\n";
}
int main(int argc, char *argv[])
{
    srand(time(NULL));
    // test();
    factorial(2000);
    return 0;
}
