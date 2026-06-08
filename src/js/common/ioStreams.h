#pragma once
#include <quickjs.h>

REF_getter<Stream> js_get_stream_CPP(JSContext *ctx, JSValueConst this_val);
bool js_IsStream(JSContext* ctx, JSValueConst val) ;
