#pragma once
#include "common/http_ptrs.h"

JSValue js_http_response_new(JSContext *ctx, const REF_getter<HTTP_ResponseP>& resp);

