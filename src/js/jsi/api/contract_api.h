#pragma once
#include "quickjs_i.h"

void register_api(JSIContext* ctx);
void delete_prop_str(JSIContext* ctx, JSIValueConst obj, const char* name);
