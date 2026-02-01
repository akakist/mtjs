#pragma once
#include <string>
#include "commonError.h"
#include <quickjs.h>
#include "stream.h"

REF_getter<Stream> js_get_stream_CPP(JSContext *ctx, JSValueConst this_val);
bool js_IsStream(JSContext* ctx, JSValueConst val) ;
