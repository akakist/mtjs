#include <string>
#include "colorOutput.h"
#include "ISSL.h"
#include "CUtils.h"
#include <iostream>
#include "IUtils.h"
#include "commonError.h"
#include "obuf.h"


template <class T1, class T2> bool ASSERT_eq(const char* file, int line, const char* func, const T1& t1, const T2& t2)
{
    if(t1!=t2)
    {
        std::cout <<ANSI_COLOR_RED " compare failed ";
        std::cout<< "t1"<< t1 << " t2 "<<t2;
        std::cout<< ANSI_COLOR_RESET;
        return true;
    }
    return false;
}

#define ASSERT_EQ(a,b) if(ASSERT_eq(__FILE__,__LINE__,__PRETTY_FUNCTION__,a,b)){return;}
// void TEST1()
// {
//     Url u;
//     u.parse("INADDR_ANY:8080");
// //    logErr2("u dump %s",u.dump().c_str());
//     ASSERT_EQ(u.protocol,"");
//     ASSERT_EQ(u.user,"");
//     ASSERT_EQ(u.pass,"");
//     ASSERT_EQ(u.host,"INADDR_ANY");
//     ASSERT_EQ(u.port,"8080");
//     ASSERT_EQ(u.dirname,"");
//     ASSERT_EQ(u.filename,"");
//     printf(GREEN("%s passed OK"),__PRETTY_FUNCTION__);

// }

// void TEST2()
// {
//     Url u;
//     u.parse("http://akakist:tipaopa@[fc20:634b:308f:a3f7:4efa:f817:8631:15e5]:22/bla/faka/blaz?aaa=1&bbb=2");
// //    logErr2("u dump %s",u.dump().c_str());
//     ASSERT_EQ(u.protocol,"http");
//     ASSERT_EQ(u.user,"akakist");
//     ASSERT_EQ(u.pass,"tipaopa");
//     ASSERT_EQ(u.host,"fc20:634b:308f:a3f7:4efa:f817:8631:15e5");
//     ASSERT_EQ(u.port,"22");
//     ASSERT_EQ(u.dirname,"/bla/faka");
//     ASSERT_EQ(u.filename,"blaz");
//     printf(GREEN("%s passed OK"),__PRETTY_FUNCTION__);
// }
// void TEST3()
// {
//     Url u;
//     u.parse("http://192.168.1.0:22/bla/faka/blaz");
//     ASSERT_EQ(u.protocol,"http");
//     ASSERT_EQ(u.user,"");
//     ASSERT_EQ(u.pass,"");
//     ASSERT_EQ(u.host,"192.168.1.0");
//     ASSERT_EQ(u.port,"22");
//     ASSERT_EQ(u.dirname,"/bla/faka");
//     ASSERT_EQ(u.filename,"blaz");
//     printf(GREEN("%s passed OK"),__PRETTY_FUNCTION__);
// }
// void TEST4()
// {
//     Url u;
//     u.parse("http://akakist:tipaopa@192.168.1.0:22/bla/faka/blaz?aaa=1&bbb=2");
//     ASSERT_EQ(u.protocol,"http");
//     ASSERT_EQ(u.user,"akakist");
//     ASSERT_EQ(u.pass,"tipaopa");
//     ASSERT_EQ(u.host,"192.168.1.0");
//     ASSERT_EQ(u.port,"22");
//     ASSERT_EQ(u.dirname,"/bla/faka");
//     ASSERT_EQ(u.filename,"blaz");
//     ASSERT_EQ(u.params,"aaa=1&bbb=2");
//     printf(GREEN("%s passed OK"),__PRETTY_FUNCTION__);

// }





template <class T> void fillrnd(I_ssl*ssl,T &z)
{
    z=rand();
    // ssl->rand_bytes((uint8_t*)&z,sizeof(z));

}
template <class T> void fillrnd(I_ssl* ssl, std::vector<T> &z)
{
    int rnd=rand()%10000;
    for(int i=0; i<rnd; i++)
    {
        int z1;
        fillrnd(ssl,z1);
        z.push_back(z1);
    }

}
template <class T> void fillrnd(I_ssl* ssl, std::deque<T> &z)
{
    int rnd=rand()%10000;
    for(int i=0; i<rnd; i++)
    {
        int z1;
        fillrnd(ssl,z1);
        z.push_back(z1);
    }

}
template <class T> void fillrnd(I_ssl* ssl, std::set<T> &z)
{
    int rnd=rand()%10000;
    for(int i=0; i<rnd; i++)
    {
        int z1;
        fillrnd(ssl,z1);
        z.insert(z1);
    }

}
template <class T1,class T2> void fillrnd(I_ssl* ssl, std::map<T1,T2> &z)
{
    int rnd=rand()%10000;
    for(int i=0; i<rnd; i++)
    {
        int z1;
        fillrnd(ssl,z1);
        int z2;
        fillrnd(ssl,z2);
        z.insert(std::make_pair(z1,z2));
    }

}


struct a
{
    std::vector<int>     n1;
    std::deque<int64_t> n2;
    std::set<int32_t> n3;
    std::vector<int8_t>  n4;
    std::deque<uint64_t> n5;
    std::map<uint32_t,uint64_t> n6;
    std::set<uint8_t>  n7;
    std::vector<long>    n8;
    std::deque<unsigned>    n9;
    std::map<unsigned  long,int> n10;
    std::deque<int64_t>    n11;
    std::set<uint16_t> n12;
    bool operator !=(const a& z) const
    {
        if(n1!=z.n1) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n2!=z.n2) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n3!=z.n3) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n4!=z.n4) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n5!=z.n5) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n6!=z.n6) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n7!=z.n7) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n8!=z.n8) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n9!=z.n9) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n10!=z.n10) {
            logErr2("fail %d",__LINE__);
            return true;
        }
        if(n11!=z.n11) return true;
        if(n12!=z.n12) return true;
        return false;
    }

    a() {
        I_ssl *ssl=(I_ssl*)iUtils->queryIface(Ifaces::SSL);
        fillrnd(ssl,n1);
        fillrnd(ssl,n2);
        fillrnd(ssl,n3);
        fillrnd(ssl,n4);
        fillrnd(ssl,n5);
        fillrnd(ssl,n6);
        fillrnd(ssl,n7);
        fillrnd(ssl,n8);
        fillrnd(ssl,n9);
        fillrnd(ssl,n10);
        fillrnd(ssl,n11);
        fillrnd(ssl,n12);
    }
};
inline outBuffer& operator<< (outBuffer& b,const a& s)
{
    b<<s.n1
     <<s.n2
     <<s.n3
     <<s.n4
     <<s.n5
     <<s.n6
     <<s.n7
     <<s.n8
     <<s.n9
     <<s.n10
     <<s.n11
     <<s.n12;
    return b;
}
inline inBuffer& operator>> (inBuffer& b,  a& s)
{
    b>>s.n1;
    b>>s.n2;
    b>>s.n3;
    b>>s.n4;
    b>>s.n5;
    b>>s.n6;
    b>>s.n7;
    b>>s.n8;
    b>>s.n9;
    b>>s.n10;
    b>>s.n11;
    b>>s.n12;
    return b;
}
inline oBuf& operator<< (oBuf& b,const a& s)
{
    b<<s.n1
     <<s.n2
     <<s.n3
     <<s.n4
     <<s.n5
     <<s.n6
     <<s.n7
     <<s.n8
     <<s.n9
     <<s.n10
     <<s.n11
     <<s.n12;
    return b;
}

struct Z
{
//    n _n;
    a _a;
    std::deque<std::string> strs;
    Z()
    {
        I_ssl *ssl=(I_ssl*)iUtils->queryIface(Ifaces::SSL);
        if(!ssl)
            throw CommonError("if(!ssl)");
        int rnd=rand()%1000;
        for(int i=0; i<rnd; i++)
        {
            int r2=rand()%1000;
            uint8_t s[r2];
            for(int j=0;j< r2;j++)
                s[j]=rand();
            strs.push_back(std::string((char*)s,sizeof(s)));
        }

    }
    bool operator !=(const Z& z) const
    {
        if(_a!=z._a) {
            logErr2("a failed");
            return true;
        }
        if(strs!=z.strs) {
            logErr2("strs failed");
            return true;
        }
        return false;
    }
    bool operator ==(const Z& z) const
    {
        return !operator !=(z);
    }
};
inline outBuffer& operator<< (outBuffer& b,const Z& s)
{
    b<<s._a;
    b<<s.strs;

    return b;
}
inline oBuf& operator<< (oBuf& b,const Z& s)
{
    b<<s._a;
    b<<s.strs;

    return b;
}
inline inBuffer& operator>> (inBuffer& b,  Z& s)
{
    b>>s._a;
//    b>>s._n;
    b>>s.strs;
    return b;
}
void TEST5()
{
//    try
//    {
    uint64_t sn[]= {
        0xFFFFFFFFFFFFFFFF,
        0xFFFFFFFFFFFFFFF,
        0xFFFFFFFFFFFFFF,
        0xFFFFFFFFFFFFF,
        0xFFFFFFFFFFFF,
        0xFFFFFFFFFFF,
        0xFFFFFFFFFF,
        0xFFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFF,
        0xFFFFFF,
        0xFFFFF,
        0xFFFF,
        0xFFF,
        0xFF,
        0xF
    };

    for(int i=0; i< sizeof(sn)/sizeof(uint64_t);  i++)
    {
        uint64_t n=sn[i];
        outBuffer o;
        o<<n;
        std::string& buf=o.buffer->container;
        inBuffer in(buf);
        int64_t nn;
        in>>nn;
        ASSERT_EQ(n,nn);
    }

    printf(GREEN("%s passed OK"),__PRETTY_FUNCTION__);

}
void TEST6()
{

    for(int i=0; i<3; i++)
    {
        printf("%s %d\n",__PRETTY_FUNCTION__,i);
        Z z;
        outBuffer o;
        o<<z;
        std::string& buf=o.buffer->container;
        inBuffer in(buf);
        Z zz;
        in >> zz;
        if(z!=zz)
        {
            ASSERT_EQ(1,2);
        }
    }

    printf(GREEN("%s passed OK"),__PRETTY_FUNCTION__);

}
void TEST66()
{
    size_t bs=3000000;
    char b[bs];
    for(int i=0; i<3; i++)
    {
        printf("%s %d\n",__PRETTY_FUNCTION__,i);
        Z z;
        oBuf o(b,bs);
        o<<z;
        std::string buf={(char*)o.data(),o.size()};
        inBuffer in(buf);
        Z zz;
        in >> zz;
        if(z!=zz)
        {
            ASSERT_EQ(1,2);
        }
    }

    printf(GREEN("%s passed OK"),__PRETTY_FUNCTION__);

}
void TEST7()
{
    I_ssl *ssl=(I_ssl*)iUtils->queryIface(Ifaces::SSL);
    int rn=rand()%10000;
    for(int i=0; i<rn; i++)
    {
        int r=rand()%10000;
        std::string str;
        for(int j=0;j<r;j++)
            str+=(char)rand();
        ASSERT_EQ(str,iUtils->Base64Decode(iUtils->Base64Encode(str)));
    }
    printf(GREEN("%s passed OK"),__PRETTY_FUNCTION__);
}
void TEST8()
{
    I_ssl *ssl=(I_ssl*)iUtils->queryIface(Ifaces::SSL);
    int rn=rand()%1000;
    for(int i=0; i<rn; i++)
    {
        int r=rand()%1000;
        std::string str;
        for(int j=0;j<r;j++)
            str+=(char)rand();
        ASSERT_EQ(str,iUtils->hex2bin(iUtils->bin2hex(str)));
    }
    printf(GREEN("%s passed OK"),__PRETTY_FUNCTION__);
}

void registerSSL(const char* pn);
int run(int ac, char** av)
{
    printf(GREEN("RUN TEST %s"),__PRETTY_FUNCTION__);
    try {
        iUtils=new CUtils(ac,av,"TestContainers");
        registerSSL(NULL);
        // TEST1();
        // TEST2();
        // TEST3();
        // TEST4();
        TEST5();
        TEST6();
        TEST66();
        TEST7();
        TEST8();
        delete iUtils;
    } catch(const CommonError&e)
    {
        printf("CommonError: %s\n",e.what());
        return 1;
    }
    return 0;


}


int _main(int ac, char** av)
{
    return run(ac,av);
}
