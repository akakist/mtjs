#pragma once
#include "httpConnection.h"
JSValue js_http_request_new(JSContext *ctx, const REF_getter<HTTP::Request>& request);
