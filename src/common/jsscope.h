#pragma once
#include "quickjs.h"
#include <string>
#include <vector>

class JSScope {
public:
    JSScope(JSContext* ctx) : ctx_(ctx) {}
    ~JSScope() {
        for (const char* str : strings_) JS_FreeCString(ctx_, str);
        for (JSValue val : values_) JS_FreeValue(ctx_, val);
    }
    const char* toCString(const JSValue& val) {
        const char* str = JS_ToCString(ctx_, val);
        if (str) strings_.push_back(str);
        return str;
    }
    std::string toStdString(const JSValue& val) {
        size_t len;
        const char* str = JS_ToCStringLen(ctx_, &len, val);
        if (str)
        {
            strings_.push_back(str);
            return std::string(str,len);
        }
        return "";
    }

    void addValue(JSValue val) {
        values_.push_back(val);
    }
private:
    JSContext* ctx_;
    std::vector<const char*> strings_;
    std::vector<JSValue> values_;
};
