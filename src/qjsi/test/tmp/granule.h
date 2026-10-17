#pragma once
#include <string>
#include "jsValueGuard.h"

struct Granule {
    JSValueGuard value;
    std::string owner_contract;
};