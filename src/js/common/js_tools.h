#pragma once
#include <quickjs.h>
#include "commonError.h"
#include "mutexInspector.h"
#include "backtrace.h"
#include "jsscope.h"
namespace qjs
{

    inline  std::string print_exception_stack(JSContext *ctx, const JSValueConst& exception)
    {
        MUTEX_INSPECTOR;
        JSScope <10,10> scope(ctx);
        JSValue stack = JS_GetPropertyStr(ctx, exception, "stack");
        scope.addValue(stack);
        auto s=scope.toStdStringView(stack);
        // JS_FreeValue(ctx,stack);
        std::string out;
        out+=(std::string)s+"\n";

        return out;
    }


    inline void checkForException(JSContext* c, const JSValue &v, const char* msg)
    {
        MUTEX_INSPECTOR;

        JSScope <10,10> scope(c);
        if (JS_IsException(v)) {
            logErr2("checkForException mark msg: %s",msg);

            JSValue exception = JS_GetException(c);
            scope.addValue(exception);
            if (JS_IsException(exception))
            {
                std::string message = (std::string)scope.toStdStringView(exception);
                logErr2("[%s] exception %s",msg,message.c_str());
            }


            auto p=print_exception_stack(c,v);
            logErr2("stack %s",p.c_str());
            // throw CommonError("checkForException");

        }

    }

    inline void convert_js_value_to_json(JSContext *ctx, JSValueConst value, std::string &result) {
        if (JS_IsString(value)) {
            size_t len;
            const char *str = JS_ToCStringLen(ctx, &len, value);
            result.append("\"").append(str).append("\"");
            JS_FreeCString(ctx, str);
        } else if (JS_IsBool(value)) {
            result.append(JS_ToBool(ctx, value) ? "true" : "false");
        } else if (JS_IsNumber(value)) {
            double num;
            JS_ToFloat64(ctx, &num, value);
            result.append(std::to_string(num));
        } else if (JS_IsArray(ctx, value)) {
            result.append("[");
            JSValue length_val = JS_GetPropertyStr(ctx, value, "length");
            uint32_t len;
            JS_ToUint32(ctx, &len, length_val);
            JS_FreeValue(ctx, length_val);
            for (uint32_t i = 0; i < len; ++i) {
                if (i > 0) result.append(", ");
                JSValue item = JS_GetPropertyUint32(ctx, value, i);
                convert_js_value_to_json(ctx, item, result);
                JS_FreeValue(ctx, item);
            }
            result.append("]");
        } else if (JS_IsObject(value)) {
            result.append("{");
            JSPropertyEnum *props;
            uint32_t len;
            JS_GetOwnPropertyNames(ctx, &props, &len, value, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK);
            for (uint32_t i = 0; i < len; ++i) {
                if (i > 0) result.append(", ");
                JSValue key = JS_AtomToValue(ctx, props[i].atom);
                JSValue val = JS_GetProperty(ctx, value, props[i].atom);
                convert_js_value_to_json(ctx, key, result);
                result.append(": ");
                convert_js_value_to_json(ctx, val, result);
                JS_FreeValue(ctx, key);
                JS_FreeValue(ctx, val);
            }
            js_free(ctx, props);
            result.append("}");
        } else {
            result.append("null");
        }
    }
    inline JSValue json_to_jsobject(JSContext *ctx, const char *json_str, size_t len)
    {
        JSValue val = JS_ParseJSON(ctx, json_str, len, "<json>");

        if (JS_IsException(val)) {
            JSValue err = JS_GetException(ctx);
            const char *err_msg = JS_ToCString(ctx, err);
            std::string error = err_msg ? err_msg : "Unknown JSON parsing error";
            JS_FreeCString(ctx, err_msg);
            JS_FreeValue(ctx, err);
            throw std::runtime_error("Failed to parse JSON: " + error);
        }

        if (!JS_IsObject(val)) {
            JS_FreeValue(ctx, val);
            throw std::runtime_error("Parsed JSON is not an object");
        }

        return val;
    }

    inline JSValue json_to_jsobject(JSContext *ctx, const std::string &json_str) {
        return json_to_jsobject(ctx, json_str.data(), json_str.size());
    }

    inline void free_promise_callbacks(JSContext *ctx, JSValue *cb)
    {
        if(!JS_IsUndefined(cb[0]))
            JS_FreeValue(ctx, cb[0]);
        if(!JS_IsUndefined(cb[1]))
            JS_FreeValue(ctx, cb[1]);
    }
    inline bool checkAndPrintException(JSContext* ctx, JSValue value, const char* where)
    {
        JSScope <10,10> scope(ctx);
        if(!JS_IsException(value)) return false;
        JSValue exc = JS_GetException(ctx);
        scope.addValue(exc);
        JSValue stack_val = JS_GetPropertyStr(ctx, exc, "stack");  // Получаем свойство stack
        scope.addValue(stack_val);
        if(!JS_IsUndefined(stack_val))
        {
            std::string stack(scope.toStdStringView(stack_val));
            printf("stack: %s\n",stack.c_str());
        }
        std::string msg(scope.toStdStringView(exc));
        printf("msg: %s\n",msg.c_str());
        return true;
    }
}