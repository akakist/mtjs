#ifndef XYJSON_TO_QUICKJS_HPP
#define XYJSON_TO_QUICKJS_HPP

#include <quickjs/quickjs.h>
#include <yyjson.h>
#include <string>
#include <vector>
#include <optional>
#include "bigint.h"

/**
 * @brief Класс для конвертации xyjson::Value в JSValue для QuickJS
 *
 * Этот класс обеспечивает рекурсивную конвертацию JSON данных из формата xyjson
 * в объекты JavaScript, понятные для движка QuickJS.
 */
class YYJsonToQuickJS {
private:
    JSContext* ctx;                    // Контекст QuickJS
    std::vector<std::string> errors;   // Хранилище ошибок конвертации

public:
    /**
     * @brief Конструктор
     * @param context Контекст QuickJS
     */
    YYJsonToQuickJS(JSContext* context) : ctx(context) {}

    /**
     * @brief Получить список ошибок
     * @return Вектор строк с сообщениями об ошибках
     */
    const std::vector<std::string>& getErrors() const {
        return errors;
    }

    /**
     * @brief Очистить список ошибок
     */
    void clearErrors() {
        errors.clear();
    }

    /**
     * @brief Основная функция конвертации xyjson::Value в JSValue
     * @param val Значение xyjson для конвертации
     * @return JSValue - объект JavaScript или JS_EXCEPTION при ошибке
     */
    JSValue convert(yyjson_val* val);

private:
    /**
     * @brief Конвертация JSON объекта
     * @param val Указатель на yyjson_val (объект)
     * @return JSValue - объект JavaScript
     */
    JSValue convertObject(yyjson_val* val);

    /**
     * @brief Конвертация JSON массива
     * @param val Указатель на yyjson_val (массив)
     * @return JSValue - массив JavaScript
     */
    JSValue convertArray(yyjson_val* val);

    /**
     * @brief Конвертация JSON строки
     * @param val Указатель на yyjson_val (строка)
     * @return JSValue - строка JavaScript
     */
    JSValue convertString(yyjson_val* val);

    /**
     * @brief Конвертация JSON числа
     * @param val Указатель на yyjson_val (число)
     * @return JSValue - число JavaScript
     */
    JSValue convertNumber(yyjson_val* val);

    /**
     * @brief Конвертация JSON булевого значения
     * @param val Указатель на yyjson_val (булево)
     * @return JSValue - булево JavaScript
     */
    JSValue convertBoolean(yyjson_val* val);

    /**
     * @brief Добавить ошибку в список
     * @param message Текст ошибки
     */
    void addError(const std::string& message) {
        errors.push_back(message);
    }
};
inline std::optional<std::string> yy_get_string(yyjson_val *params, const char *key, std::string& out)
{
    auto _name=yyjson_obj_get(params,key);
    if(!_name)
        return "param '"+(std::string)key+"' must be specified";

    if(!yyjson_is_str(_name))
        return "'"+(std::string)key+"' must be string";
    out= yyjson_get_str(_name);
    return std::nullopt;

}
inline std::optional<std::string> yy_get_bn(yyjson_val *params, const char *key, BigInt& out)
{
    auto _name=yyjson_obj_get(params,key);
    if(!_name)
        return "param '"+(std::string)key+"' must be specified";

    if(yyjson_is_str(_name))
        out.from_string(yyjson_get_str(_name));
    else if(yyjson_is_num(_name))
        out=yyjson_get_uint(_name);
    else return "'"+(std::string)key+"' must be string or num";

    return std::nullopt;

}

#endif // XYJSON_TO_QUICKJS_HPP
