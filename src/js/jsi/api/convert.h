#pragma once
#include "quickjs_i.h"
#include <string>
#include <nlohmann/json.hpp>

std::string serialize_value(JSIContext* ctx, JSIValueConst val, int depth = 0);
bool is_serializable(const std::string& s);
nlohmann::json json_from_jsi(JSIContext* ctx, JSIValueConst val);
JSIValue json_to_jsi(JSIContext* ctx, const nlohmann::json& j);
JSIValue convert_value(JSIContext* from_ctx, JSIContext* to_ctx, JSIValueConst val);
void print_exception(JSIContext* ctx);
