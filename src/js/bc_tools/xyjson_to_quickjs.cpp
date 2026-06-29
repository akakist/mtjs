#include "xyjson_to_quickjs.hpp"
#include <cstring>
#include <cmath>

/**
 * @brief Основная функция конвертации - определяет тип и вызывает соответствующий обработчик
 *
 * @param val Значение xyjson для конвертации
 * @return JSValue - результат конвертации
 */
JSValue XYJsonToQuickJS::convert(const yyjson::Value& val) {
    // Получаем сырой указатель на структуру yyjson
    // xyjson::Value хранит внутри себя yyjson_val*
    yyjson_val* raw_val = val.get();

    // Проверяем, что указатель валидный
    if (!raw_val) {
        addError("Ошибка: нулевой указатель yyjson_val");
        return JS_ThrowInternalError(ctx, "Null yyjson value");
    }

    // Получаем тип JSON значения через C API
    yyjson_type type = yyjson_get_type(raw_val);

    // В зависимости от типа вызываем соответствующую функцию конвертации
    switch (type) {
    case YYJSON_TYPE_OBJ:   // Объект
        return convertObject(raw_val);

    case YYJSON_TYPE_ARR:   // Массив
        return convertArray(raw_val);

    case YYJSON_TYPE_STR:   // Строка
        return convertString(raw_val);

    case YYJSON_TYPE_NUM:   // Число
        return convertNumber(raw_val);

    case YYJSON_TYPE_BOOL:  // Булево
        return convertBoolean(raw_val);

    case YYJSON_TYPE_NULL:  // NULL
        return JS_NULL;

    default:                // Неизвестный тип
        addError("Неизвестный тип JSON: " + std::to_string(static_cast<int>(type)));
        return JS_UNDEFINED;
    }
}

/**
 * @brief Конвертация JSON объекта в объект JavaScript
 *
 * Создает пустой объект, затем итерирует по всем полям исходного JSON объекта,
 * рекурсивно конвертирует каждое значение и добавляет его в объект.
 *
 * @param val Указатель на yyjson_val (должен быть типа YYJSON_TYPE_OBJ)
 * @return JSValue - объект JavaScript
 */
JSValue XYJsonToQuickJS::convertObject(yyjson_val* val) {
    // Проверяем, что передан именно объект
    if (!val || yyjson_get_type(val) != YYJSON_TYPE_OBJ) {
        addError("Ошибка: ожидался JSON объект");
        return JS_ThrowTypeError(ctx, "Expected JSON object");
    }

    // Создаем пустой объект в QuickJS
    JSValue js_obj = JS_NewObject(ctx);
    if (JS_IsException(js_obj)) {
        addError("Не удалось создать объект JavaScript");
        return JS_EXCEPTION;
    }

    // Инициализируем итератор для обхода полей объекта
    yyjson_obj_iter iter;
    yyjson_obj_iter_init(val, &iter);

    // Переменные для ключа и значения
    yyjson_val* key;
    yyjson_val* value;

    // Обходим все поля объекта
    while ((key = yyjson_obj_iter_next(&iter))) {
        // Получаем значение для текущего ключа
        value = yyjson_obj_iter_get_val(key);

        // Извлекаем строку ключа
        const char* key_str = yyjson_get_str(key);
        if (!key_str) {
            // Если ключ не является строкой (что маловероятно в JSON)
            // пропускаем этот элемент
            addError("Пропущен элемент с нестроковым ключом");
            continue;
        }

        // Рекурсивно конвертируем значение
        // Для этого создаем временный объект xyjson::Value из сырого указателя
        JSValue js_value = convert(yyjson::Value(value));

        // Проверяем, не произошла ли ошибка при конвертации
        if (JS_IsException(js_value)) {
            // Освобождаем созданный объект и возвращаем ошибку
            JS_FreeValue(ctx, js_obj);
            addError("Ошибка конвертации поля: " + std::string(key_str));
            return JS_EXCEPTION;
        }

        // Устанавливаем свойство в объект JavaScript
        // Функция JS_SetPropertyStr автоматически освобождает js_value
        // при успешной установке (передает владение объекту)
        JS_SetPropertyStr(ctx, js_obj, key_str, js_value);
    }

    // Возвращаем созданный объект
    return js_obj;
}

/**
 * @brief Конвертация JSON массива в массив JavaScript
 *
 * Создает пустой массив, затем итерирует по всем элементам исходного JSON массива,
 * рекурсивно конвертирует каждый элемент и добавляет его в массив.
 *
 * @param val Указатель на yyjson_val (должен быть типа YYJSON_TYPE_ARR)
 * @return JSValue - массив JavaScript
 */
JSValue XYJsonToQuickJS::convertArray(yyjson_val* val) {
    // Проверяем, что передан именно массив
    if (!val || yyjson_get_type(val) != YYJSON_TYPE_ARR) {
        addError("Ошибка: ожидался JSON массив");
        return JS_ThrowTypeError(ctx, "Expected JSON array");
    }

    // Создаем пустой массив в QuickJS
    JSValue js_arr = JS_NewArray(ctx);
    if (JS_IsException(js_arr)) {
        addError("Не удалось создать массив JavaScript");
        return JS_EXCEPTION;
    }

    // Инициализируем итератор для обхода элементов массива
    yyjson_arr_iter iter;
    yyjson_arr_iter_init(val, &iter);

    // Индекс текущего элемента
    uint32_t index = 0;
    yyjson_val* item;

    // Обходим все элементы массива
    while ((item = yyjson_arr_iter_next(&iter))) {
        // Рекурсивно конвертируем элемент
        JSValue js_item = convert(yyjson::Value(item));

        // Проверяем, не произошла ли ошибка
        if (JS_IsException(js_item)) {
            // Освобождаем созданный массив
            JS_FreeValue(ctx, js_arr);
            addError("Ошибка конвертации элемента массива с индексом: " + std::to_string(index));
            return JS_EXCEPTION;
        }

        // Устанавливаем элемент в массив по индексу
        // Функция JS_SetPropertyUint32 передает владение js_item массиву
        JS_SetPropertyUint32(ctx, js_arr, index++, js_item);
    }

    // Возвращаем созданный массив
    return js_arr;
}

/**
 * @brief Конвертация JSON строки в строку JavaScript
 *
 * @param val Указатель на yyjson_val (должен быть типа YYJSON_TYPE_STR)
 * @return JSValue - строка JavaScript
 */
JSValue XYJsonToQuickJS::convertString(yyjson_val* val) {
    // Проверяем, что передан именно строка
    if (!val || yyjson_get_type(val) != YYJSON_TYPE_STR) {
        addError("Ошибка: ожидалась JSON строка");
        return JS_ThrowTypeError(ctx, "Expected JSON string");
    }

    // Получаем указатель на строку и ее длину
    const char* str = yyjson_get_str(val);
    size_t len = yyjson_get_len(val);

    // Создаем строку в QuickJS
    JSValue js_str = JS_NewStringLen(ctx, str, len);
    if (JS_IsException(js_str)) {
        addError("Не удалось создать строку JavaScript");
        return JS_ThrowInternalError(ctx, "Failed to create JS string");
    }

    return js_str;
}

/**
 * @brief Конвертация JSON числа в число JavaScript
 *
 * Определяет, является ли число целым или с плавающей точкой,
 * и создает соответствующий тип в QuickJS.
 *
 * @param val Указатель на yyjson_val (должен быть типа YYJSON_TYPE_NUM)
 * @return JSValue - число JavaScript
 */
JSValue XYJsonToQuickJS::convertNumber(yyjson_val* val) {
    // Проверяем, что передан именно число
    if (!val || yyjson_get_type(val) != YYJSON_TYPE_NUM) {
        addError("Ошибка: ожидалось JSON число");
        return JS_ThrowTypeError(ctx, "Expected JSON number");
    }

    // Проверяем, является ли число целым (int)
    if (yyjson_is_int(val)) {
        // Извлекаем целочисленное значение
        long long int_val = yyjson_get_int(val);

        // Создаем целое число в QuickJS
        JSValue js_num = JS_NewInt64(ctx, int_val);
        if (JS_IsException(js_num)) {
            addError("Не удалось создать целое число JavaScript");
            return JS_ThrowInternalError(ctx, "Failed to create JS integer");
        }
        return js_num;
    }
    else {
        // Извлекаем число с плавающей точкой
        double double_val = yyjson_get_real(val);

        // Проверяем на специальные значения
        if (std::isnan(double_val) || std::isinf(double_val)) {
            addError("Обнаружено невалидное число (NaN или Infinity)");
            return JS_ThrowInternalError(ctx, "Invalid number: NaN or Infinity");
        }

        // Создаем число с плавающей точкой в QuickJS
        JSValue js_num = JS_NewFloat64(ctx, double_val);
        if (JS_IsException(js_num)) {
            addError("Не удалось создать число с плавающей точкой JavaScript");
            return JS_ThrowInternalError(ctx, "Failed to create JS float");
        }
        return js_num;
    }
}

/**
 * @brief Конвертация JSON булева значения в булево JavaScript
 *
 * @param val Указатель на yyjson_val (должен быть типа YYJSON_TYPE_BOOL)
 * @return JSValue - булево JavaScript
 */
JSValue XYJsonToQuickJS::convertBoolean(yyjson_val* val) {
    // Проверяем, что передан именно булево
    if (!val || yyjson_get_type(val) != YYJSON_TYPE_BOOL) {
        addError("Ошибка: ожидалось JSON булево значение");
        return JS_ThrowTypeError(ctx, "Expected JSON boolean");
    }

    // Извлекаем булево значение
    bool bool_val = yyjson_get_bool(val);

    // Создаем булево в QuickJS
    return JS_NewBool(ctx, bool_val);
}