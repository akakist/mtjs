#include "mcrypto.h"
#include "base62.h"
int main()
{
    auto k=mcrypto::generate_keypair();
    auto p=base62::encode(k.first);
    auto s=base62::encode(k.second);

    printf("pk:%s\nsk:%s",p.c_str(),s.c_str());
    
}