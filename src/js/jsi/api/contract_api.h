#pragma once
#include "quickjs_i.h"

void register_api(JSIContext* ctx);
void delete_prop_str(JSIContext* ctx, JSIValueConst obj, const char* name);
JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& m);
JSIValue build_view_ctx(JSIContext* ctx);