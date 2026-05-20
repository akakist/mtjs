#pragma once
#include "commonError.h"
const char* msgName(int id);

namespace MsgData
{
class Base;
}

class MsgFactory {
public:
    using Constructor = MsgData::Base* (*)();
    
    MsgData::Base* create(const int& id) {

        auto it = registry.find(id);
        if(it==registry.end())
        {
            throw CommonError("Message type not found %d %s", id,msgName(id));
        }
        return it->second();
    }
    
    bool registerMsg(const int& id, Constructor ctor) {
        registry[id] = ctor;
        return true;
    }
    MsgFactory();
    
private:
        std::map<int, Constructor> registry;
};
