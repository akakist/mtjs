#include "granularMap.h"
#include <cstring>

JSClassID js_granular_map_class_id;

// ===== Finalizer (освобождаем память) =====
static void js_granular_map_finalizer(JSRuntime* rt, JSValue val) {
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(val, js_granular_map_class_id));
    if (map) {
        delete map;
    }
}

// ===== Класс для QuickJS =====
static JSClassDef js_granular_map_class = {
    "GranularMap",
    .finalizer = js_granular_map_finalizer,
};

// ===== Конструктор (с параметром int) =====
static JSValue js_granular_map_constructor(JSContext* ctx, JSValueConst this_val,
                                           int argc, JSValueConst* argv) {
    int keySize = 20; // значение по умолчанию
    
    // Если передан аргумент — используем его как keySize
    if (argc >= 1) {
        if (JS_ToInt32(ctx, &keySize, argv[0])) {
            return JS_ThrowTypeError(ctx, "GranularMap: argument must be a number");
        }
        if (keySize < 1 || keySize > 64) {
            return JS_ThrowTypeError(ctx, "GranularMap: keySize must be between 1 and 64");
        }
    }
    
    // Создаём объект C++
    JS_GranularMap* map = new JS_GranularMap(ctx, keySize);
    
    // Создаём JS-объект и привязываем C++-объект
    JSValue obj = JS_NewObjectClass(ctx, js_granular_map_class_id);
    JS_SetOpaque(obj, map);
    
    return obj;
}

// ===== Метод set(key, value) =====
static JSValue js_granular_map_set(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(this_val, js_granular_map_class_id));
    if (!map) {
        return JS_ThrowTypeError(ctx, "GranularMap.set: invalid object");
    }
    
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "GranularMap.set: expected 2 arguments (key, value)");
    }
    
    const char* key = JS_ToCString(ctx, argv[0]);
    const char* value = JS_ToCString(ctx, argv[1]);
    
    if (!key || !value) {
        JS_FreeCString(ctx, key);
        JS_FreeCString(ctx, value);
        return JS_ThrowTypeError(ctx, "GranularMap.set: key and value must be strings");
    }
    
    map->set(key, value);
    
    JS_FreeCString(ctx, key);
    JS_FreeCString(ctx, value);
    
    return JS_UNDEFINED;
}

// ===== Метод get(key) =====
static JSValue js_granular_map_get(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(this_val, js_granular_map_class_id));
    if (!map) {
        return JS_ThrowTypeError(ctx, "GranularMap.get: invalid object");
    }
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "GranularMap.get: expected 1 argument (key)");
    }
    
    const char* key = JS_ToCString(ctx, argv[0]);
    if (!key) {
        return JS_ThrowTypeError(ctx, "GranularMap.get: key must be a string");
    }
    
    std::string value = map->get(key);
    JS_FreeCString(ctx, key);
    
    return JS_NewString(ctx, value.c_str());
}

// ===== Метод del(key) =====
static JSValue js_granular_map_del(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(this_val, js_granular_map_class_id));
    if (!map) {
        return JS_ThrowTypeError(ctx, "GranularMap.del: invalid object");
    }
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "GranularMap.del: expected 1 argument (key)");
    }
    
    const char* key = JS_ToCString(ctx, argv[0]);
    if (!key) {
        return JS_ThrowTypeError(ctx, "GranularMap.del: key must be a string");
    }
    
    map->del(key);
    JS_FreeCString(ctx, key);
    
    return JS_UNDEFINED;
}

// ===== Метод has(key) =====
static JSValue js_granular_map_has(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(this_val, js_granular_map_class_id));
    if (!map) {
        return JS_ThrowTypeError(ctx, "GranularMap.has: invalid object");
    }
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "GranularMap.has: expected 1 argument (key)");
    }
    
    const char* key = JS_ToCString(ctx, argv[0]);
    if (!key) {
        return JS_ThrowTypeError(ctx, "GranularMap.has: key must be a string");
    }
    
    bool exists = map->data.find(key) != map->data.end();
    JS_FreeCString(ctx, key);
    
    return JS_NewBool(ctx, exists);
}

// ===== Метод size() =====
static JSValue js_granular_map_size(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv) {
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(this_val, js_granular_map_class_id));
    if (!map) {
        return JS_ThrowTypeError(ctx, "GranularMap.size: invalid object");
    }
    
    return JS_NewInt64(ctx, map->data.size());
}

// ===== Метод keys() =====
static JSValue js_granular_map_keys(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv) {
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(this_val, js_granular_map_class_id));
    if (!map) {
        return JS_ThrowTypeError(ctx, "GranularMap.keys: invalid object");
    }
    
    JSValue arr = JS_NewArray(ctx);
    int i = 0;
    for (const auto& pair : map->data) {
        JSValue key = JS_NewString(ctx, pair.first.c_str());
        JS_SetPropertyUint32(ctx, arr, i++, key);
    }
    
    return arr;
}

// ===== Список методов класса =====
static const JSCFunctionListEntry js_granular_map_proto_funcs[] = {
    JS_CFUNC_DEF("set", 2, js_granular_map_set),
    JS_CFUNC_DEF("get", 1, js_granular_map_get),
    JS_CFUNC_DEF("del", 1, js_granular_map_del),
    JS_CFUNC_DEF("has", 1, js_granular_map_has),
    JS_CFUNC_DEF("size", 0, js_granular_map_size),
    JS_CFUNC_DEF("keys", 0, js_granular_map_keys),
};

// ===== Регистрация класса в QuickJS =====
void register_granular_map_class(JSContext* ctx) {
    JS_NewClassID(&js_granular_map_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_granular_map_class_id, &js_granular_map_class);

    // Создаём прототип
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_granular_map_proto_funcs, 
                               sizeof(js_granular_map_proto_funcs) / sizeof(JSCFunctionListEntry));
    JS_SetClassProto(ctx, js_granular_map_class_id, proto);

    // Регистрируем глобальный конструктор
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue constructor = JS_NewCFunction2(ctx, js_granular_map_constructor, "GranularMap", 1, JS_CFUNC_constructor, 0);
    JS_SetPropertyStr(ctx, global, "GranularMap", constructor);
    JS_FreeValue(ctx, global);
}

// ===== Реализация методов C++ =====

JS_GranularMap::JS_GranularMap(JSContext* ctx, int keySize)
    : ctx(ctx), keySize(keySize) {
    // Можно добавить аллокатор или что-то ещё
}

JS_GranularMap::~JS_GranularMap() {
    // Очистка (данные удаляются автоматически)
}

void JS_GranularMap::set(const std::string& key, const std::string& value) {
    data[key] = value;
}

std::string JS_GranularMap::get(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end()) {
        return it->second;
    }
    return "";
}

void JS_GranularMap::del(const std::string& key) {
    data.erase(key);
}