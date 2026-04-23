#pragma once
#include <string>
#include "ghash.h"
#include "ioBuffer.h"
#include <nlohmann/json.hpp>
inline std::string json_dump(const nlohmann::json &j)
{
    return j.dump(4);
}
inline nlohmann::json json_parse(const std::string &s)
{
    return nlohmann::json::parse(s);
}
