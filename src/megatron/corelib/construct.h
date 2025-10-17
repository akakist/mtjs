#pragma once
#include <string.h>
#include "SERVICE_id.h"
namespace Event {
    class Base;

}
class route_t;
class UnknownBase;
/// Static constructor, needed only for serializebla events
typedef Event::Base* (*event_static_constructor) (const route_t& r);

class IInstance;
typedef UnknownBase* (*unknown_static_constructor) (const SERVICE_id& id, const std::string& name,IInstance* ifa);
