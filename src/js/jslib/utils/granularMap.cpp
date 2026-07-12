#include "granularMap.h"
#include <cstring>
#include "contract_rt.h"
#include "jsscope.h"

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

    contract_rt *cc=(contract_rt *)JS_GetContextOpaque(ctx);
    if(!cc)
        return JS_ThrowTypeError(ctx, "GranularMap.constructor: invalid object");

    JSScope<20,20> scope(ctx);
    if(argc!=1)
        return JS_ThrowSyntaxError(ctx,"argc must be 1");
    auto name=scope.toStdString(argv[0]);
    
    
    // Создаём объект C++
    JS_GranularMap* map = new JS_GranularMap(ctx, name,cc);
    
    // Создаём JS-объект и привязываем C++-объект
    JSValue obj = JS_NewObjectClass(ctx, js_granular_map_class_id);
    JS_SetOpaque(obj, map);
    
    return obj;
}

// ===== Метод set(key, value) =====
static JSValue js_granular_map_set(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) 
{
    JSScope<20,20> scope(ctx);
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(this_val, js_granular_map_class_id));
    if (!map) {
        return JS_ThrowTypeError(ctx, "GranularMap.set: invalid object");
    }
    contract_rt *cc=(contract_rt *)JS_GetContextOpaque(ctx);
    if(!cc)
        return JS_ThrowTypeError(ctx, "GranularMap.set: invalid contract_rt");

        
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "GranularMap.set: expected 2 arguments (key:string, value:object)");
    }
    if(!JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "GranularMap.set: arg0 must be string");
    if(!JS_IsObject(argv[1]))
        return JS_ThrowTypeError(ctx, "GranularMap.set: arg1 must be object");

    std::string key=scope.toStdString(argv[0]);
    size_t bufsiz=0;    
    auto *buf=JS_WriteObject(ctx,&bufsiz,argv[1],0);
    std::string buff{(char*)buf,bufsiz};
    js_free(ctx, buf);

    CONTRACT_DATA_id cid;
    cid.container="c."+cc->name.container+"."+map->name+"."+key;
    auto d=cc->root->getContractData(cid);
    if(!d.valid());
        return JS_ThrowInternalError(ctx, "granule not exists for update");
    d->container=buff;
    
    // auto ret=JS_ReadObject(ctx,(uint8_t*)d->container.data(),d->container.size(),0);

    // return ret;
    
    return JS_UNDEFINED;
}

// ===== Метод get(key) =====
static JSValue js_granular_map_get(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) 
{
    JSScope<20,20> scope(ctx);
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(this_val, js_granular_map_class_id));
    if (!map) {
        return JS_ThrowTypeError(ctx, "GranularMap.get: invalid object");
    }
    contract_rt *cc=(contract_rt *)JS_GetContextOpaque(ctx);
    if(!cc)
        return JS_ThrowTypeError(ctx, "GranularMap.set: invalid contract_rt");
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "GranularMap.get: expected 1 argument (key)");
    }
    if(!JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "GranularMap.get: key must be a string");

    
    auto key=scope.toStdString(argv[0]);

    CONTRACT_DATA_id cid;
    cid.container="c."+cc->name.container+"."+map->name+"."+key;
    auto d=cc->root->getContractData(cid);
    if(!d.valid());
        return JS_ThrowInternalError(ctx, "granule not exists for get");
    
    // std::string value = map->get(key);
    // JS_FreeCString(ctx, key);
    auto ret=JS_ReadObject(ctx,(uint8_t*)d->container.data(),d->container.size(),0);

    return ret;
    
    // return JS_NewString(ctx, "kall");
}


// ===== Метод has(key) =====
static JSValue js_granular_map_has(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    JSScope<20,20> scope(ctx);
    JS_GranularMap* map = static_cast<JS_GranularMap*>(JS_GetOpaque(this_val, js_granular_map_class_id));
    if (!map) {
        return JS_ThrowTypeError(ctx, "GranularMap.has: invalid object");
    }
    contract_rt *cc=(contract_rt *)JS_GetContextOpaque(ctx);
    if(!cc)
        return JS_ThrowTypeError(ctx, "GranularMap.has: invalid contract_rt");
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "GranularMap.has: expected 1 argument (key)");
    }
    auto key=scope.toStdString(argv[0]);

    CONTRACT_DATA_id cid;
    cid.container="c."+cc->name.container+"."+map->name+"."+key;
    auto d=cc->root->getContractData(cid);
    if(d.valid())
        return JS_NewBool(ctx,true);
    else 
        return JS_NewBool(ctx,false);
        // return JS_ThrowInternalError(ctx, "granule not exists for get");

    
    // bool exists = map->data.find(key) != map->data.end();
    // JS_FreeCString(ctx, key);
    
    return JS_NewBool(ctx, false);
}



// ===== Список методов класса =====
static const JSCFunctionListEntry js_granular_map_proto_funcs[] = {
    JS_CFUNC_DEF("set", 2, js_granular_map_set),
    JS_CFUNC_DEF("get", 1, js_granular_map_get),
    JS_CFUNC_DEF("has", 1, js_granular_map_has),
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

JS_GranularMap::JS_GranularMap(JSContext* ctx, const std::string& _name, const REF_getter<contract_rt> & _contract)
    : ctx(ctx), name(_name), contract(_contract) {
    // Можно добавить аллокатор или что-то ещё
}

JS_GranularMap::~JS_GranularMap() {
    // Очистка (данные удаляются автоматически)
}

// void JS_GranularMap::set(const std::string& key, const std::string& value) {
//     // data[key] = value;
// }

// std::string JS_GranularMap::get(const std::string& key) 
// {
//     CONTRACT_DATA_id cid;
//     // cid.container=contract->name.container+"."+name+"."+key;
//     // auto v=contract->root->getContractData(cid);
//     // if(!v.valid())
//     // return NU
//     // auto it = data.find(key);
//     // if (it != data.end()) {
//     //     return it->second;
//     // }
//     return "";
// }

// void JS_GranularMap::del(const std::string& key) {
//     // data.erase(key);
// }