#include <iostream>
#include "httplib.h"
#include "msg.h"
std::string hex2bin(const std::string &s)
{
    std::string out="";
    out.reserve(s.size());
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

std::string bin2hex(const std::string & in)
{
    std::string out = "";
    out.reserve(in.size()*2);
    const unsigned char *p = (unsigned char *)in.data();

    for (unsigned int i = 0; i < in.size(); i++)
    {
        XTRY;
        char s[40];
        ::snprintf(s, sizeof(s) - 1, "%02x", p[i]);
        out += s;
        XPASS;
    }
    return out;
}

int main() {
    // создаём клиента для сервера
    httplib::Client cli("http://localhost:8080");

    // тело запроса
    std::string body = R"({"name":"Sergey","msg":"Hello from httplib"})";

    // заголовки
    httplib::Headers headers = {
        { "Content-Type", "application/json" }
    };

    std::string _sk="d7709468e4d1e374df6abebd6e7e0eee1a5bfceba694a20ebde46fae2ff5130d";
    bls::SecretKey sk;
    sk.deserializeHexStr(_sk);
    bls::PublicKey pk;
    sk.getPublicKey(pk);
    
    auto *m=new registerNode(msg::registerNode);
    m->pk=hex2bin(pk.serializeToHexStr());
    m->ip="127.0.0.1:5555";
    m->name="serge";

    // отправляем POST
    auto res = cli.Post("/api/test", headers, body, "application/json");

    if (res) {
        std::cout << "Status: " << res->status << std::endl;
        std::cout << "Body: " << res->body << std::endl;
    } else {
        std::cerr << "Request failed" << std::endl;
    }

    return 0;
}
