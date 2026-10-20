#pragma once
#include "quickjs_i.h"
#include <string>
#include <string_view>

template<size_t S1, size_t S2>
class JSScope {
public:
    JSScope(JSContext* ctx) : ctx_(ctx) {}
    ~JSScope() {
        for(size_t i=0; i<stringgs_n; i++)
            JSI_FreeCString(ctx_, strings_[i]);
        for(size_t i=0; i<values_n; i++)
            JSI_FreeValue(ctx_, values_[i]);
    }
    std::string_view toStdStringView(const JSValue& val) {
        size_t len;
        const char* str = JSI_ToCStringLen(ctx_, &len, val);
        if (str)
        {
            if(stringgs_n<S1)
            {
                strings_[stringgs_n++]=str;
            }
            else throw std::runtime_error("!if(stringgs_n<S1)");

            return std::string_view(str,len);
        }
        return "";
    }
    std::string toStdString(const JSValue& val) {
        size_t len;
        const char* str = JSI_ToCStringLen(ctx_, &len, val);
        if (str)
        {
            if(stringgs_n<S1)
            {
                strings_[stringgs_n++]=str;
            }
            else throw std::runtime_error("!if(stringgs_n<S1)");

            return std::string(str,len);
        }
        return "";
    }

    void addValue(JSValue val) {

        if(values_n<S2)
        {
            values_[values_n++]=val;
        }
        else throw std::runtime_error("!if(values_n<S2)");
    }
private:
    JSContext* ctx_;
    const char* strings_[S1];
    size_t stringgs_n=0;
    JSValue values_[S2];
    size_t values_n=0;
};
