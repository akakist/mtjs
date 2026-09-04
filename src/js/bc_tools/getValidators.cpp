#include "root_contract.h"
#include "IDatabase.h"
uint64_t fnv1a_64(const void* buffer, size_t len) 
{
    const unsigned char* data = (const unsigned char*)buffer;
    uint64_t hash = 0xCBF29CE484222325ULL; // FNV-64 offset basis
    
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 0x100000001B3ULL; // FNV-64 prime
    }
    
    return hash;
}
std::set<NODE_id> getValidators(uint64_t block_timestamp, IDatabase* db)
{
    auto ns=db->getNodeListNoCreate(db);
    if(!ns.valid())
        throw CommonError("if(!ns.valid())");
    auto v=db->getValuesNoCreate(db);
    if(!v.valid())
        throw CommonError("if(!v.valid()");
    // auto n_validators=
    std::map<uint64_t,std::set<NODE_id> > res;
    auto ts=std::to_string(block_timestamp);
    auto l=ns->getList();
    for(auto& z: l)
    {
        auto s=z.container+ts;
        auto h=fnv1a_64(s.data(),s.size());
        auto node=db->getNode(z,db);
        h/=node->get_full_stake()+1;
        res[h].insert(z);
    }
    // int idx=0;
    std::set<NODE_id> out;
    for(auto& z:res)
    {
        if(out.size() > v->validator_count)
            break;
        for(auto &x:z.second)
        {
            if(out.size() > v->validator_count)
                break;
            out.insert(x);

        }
    }
    return out;

}