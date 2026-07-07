#pragma once
#include <quickjs.h>
#include <unordered_map>
#include <string>

// Класс-обёртка для хранения данных (аналог JS_HttpServer)
class JS_GranularMap {
public:
    std::unordered_map<std::string, std::string> data;
    JSContext* ctx;
    
    // Конструктор с параметром int (аналог порта — размер ключа)
    int keySize; // например, 4, 8, 20 байт
    
    JS_GranularMap(JSContext* ctx, int keySize = 20);
    ~JS_GranularMap();
    
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    void del(const std::string& key);
};

// ID класса для QuickJS
extern JSClassID js_granular_map_class_id;

// Регистрация класса в QuickJS
void register_granular_map_class(JSContext* ctx);
