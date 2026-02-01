#pragma once
#include "quickjs.h"
#include <string>
#include <string_view>
#include <stdexcept>
#include "commonError.h"

template<size_t S1, size_t S2>
class JSScope {
public:
    JSScope(JSContext* ctx) : ctx_(ctx) {}
    ~JSScope() {
        for(size_t i=0;i<stringgs_n;i++)
            JS_FreeCString(ctx_, strings_[i]);
        for(size_t i=0;i<values_n;i++)
            JS_FreeValue(ctx_, values_[i]);
    }
    std::string_view toStdStringView(const JSValue& val) {
        size_t len;
        const char* str = JS_ToCStringLen(ctx_, &len, val);
        if (str)
        {
            if(stringgs_n<S1)
            {
                strings_[stringgs_n++]=str;    
            }
            else throw CommonError("!if(stringgs_n<S1)"+_DMI());
                
            return std::string_view(str,len);
        }
        return "";
    }

    void addValue(JSValue val) {
        
        if(values_n<S2)
        {
            values_[values_n++]=val;
        }
        else throw CommonError("!if(values_n<S2)"+_DMI());
    }
private:
    JSContext* ctx_;
    const char* strings_[S1];
    size_t stringgs_n=0;
    JSValue values_[S2];
    size_t values_n=0;
};
