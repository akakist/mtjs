#pragma once
#include <quickjs.h>
#include <unordered_map>
#include <string>
#include "REF.h"
#include "contract_rt.h"

// Класс-обёртка для хранения данных (аналог JS_HttpServer)
class JS_GranularMap {
public:
    std::string name;
    JSContext* ctx;
    REF_getter<contract_rt> contract;
    
    // Конструктор с параметром int (аналог порта — размер ключа)
    // int keySize; // например, 4, 8, 20 байт
    
    JS_GranularMap(JSContext* ctx, const std::string& name_, const REF_getter<contract_rt>& _contract);
    ~JS_GranularMap();
    
    // void set(const std::string& key, const std::string& value);
    // std::string get(const std::string& key);
    // void del(const std::string& key);
};

// ID класса для QuickJS
extern JSClassID js_granular_map_class_id;

// Регистрация класса в QuickJS
void register_granular_map_class(JSContext* ctx);
