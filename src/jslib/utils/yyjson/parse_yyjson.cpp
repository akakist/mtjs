#include "yyjson.h"
#include <quickjs.h>
#include "IUtils.h"
#include "commonError.h"
#include "malloc_debug.h"
JSValue convert_yyjson_to_js(JSContext *ctx, yyjson_val *val);

// Функция для парсинга JSON-строки и создания JSObject в QuickJS
extern "C"
JSValue parse_yyjson(JSContext *ctx, const char *json_str, size_t len) 
{
    yyjson_doc *doc = yyjson_read(json_str, len, 0);
    // DBG(iUtils->mem_add_ptr("yyjson",doc));
    if (!doc) {
        return JS_EXCEPTION;
    }
    

    yyjson_val *root = yyjson_doc_get_root(doc);
    JSValue result = convert_yyjson_to_js(ctx, root);
    DBG(memctl_add_object(result,"convert_yyjson_to_js"));
    yyjson_doc_free(doc);
    // DBG(iUtils->mem_remove_ptr("yyjson",doc));
    return result;
}

// Вспомогательная функция для конвертации yyjson_val в JSValue
JSValue convert_yyjson_to_js(JSContext *ctx, yyjson_val *val) 
{
    if (!val) {
        return JS_NULL;
    }

    switch (yyjson_get_type(val)) {
        case YYJSON_TYPE_NULL:
            return JS_NULL;
        case YYJSON_TYPE_BOOL:
            return JS_NewBool(ctx, yyjson_get_bool(val));
        case YYJSON_TYPE_NUM: {
            if (yyjson_is_int(val)) {
                return JS_NewInt64(ctx, yyjson_get_int(val));
            } else {
                return JS_NewFloat64(ctx, yyjson_get_real(val));
            }
        }
        case YYJSON_TYPE_STR:
            return JS_NewString(ctx, yyjson_get_str(val));
        case YYJSON_TYPE_ARR: {
            size_t idx, max;
            yyjson_val *item;
            JSValue arr = JS_NewArray(ctx);
                                            DBG(memctl_add_object(arr, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            yyjson_arr_foreach(val, idx, max, item) {
                JS_SetPropertyUint32(ctx, arr, idx, convert_yyjson_to_js(ctx, item));
            }
            return arr;
        }
        case YYJSON_TYPE_OBJ: {
            JSValue obj = JS_NewObject(ctx);
                                            DBG(memctl_add_object(obj, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            yyjson_val *key, *value;
            size_t idx, max;
            yyjson_obj_foreach(val, idx, max, key, value) {
                JS_DefinePropertyValueStr(ctx, obj, yyjson_get_str(key), convert_yyjson_to_js(ctx, value), JS_PROP_C_W_E);
            }
            return obj;
        }
        default:
            return JS_UNDEFINED;
    }
}
