#include <quickjsi_i.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <quickjsi_i.h>
#include <string>
#include <map>
#include <iostream>
#include "jsValueGuard.h"
#include "jsscope.h"
#include <set>


#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>

// Глобальное состояние (для имитации)
std::map<std::string, JSValueGuard> granules;
std::map<std::string, JSValueGuard> methods;
std::map<std::string, JSValueGuard> views;
std::map<std::string, JSContext*> loaded_contracts;

// Читает каталог и возвращает имена файлов (без пути), отсортированные.
// Только регулярные файлы. При ошибке — пустой вектор.
std::vector<std::string> list_directory(const std::string& path) {
    std::vector<std::string> result;

    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return result;  // пустой вектор при ошибке
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // пропускаем "." и ".."
        if (std::strcmp(entry->d_name, ".") == 0 ||
            std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // проверяем, что это регулярный файл
        std::string fullpath = path + "/" + entry->d_name;
        struct stat st;
        if (stat(fullpath.c_str(), &st) != 0) {
            continue;
        }
        if (!S_ISREG(st.st_mode)) {
            continue;
        }

        result.emplace_back(entry->d_name);
    }
    closedir(dir);

    // сортируем для детерминизма
    std::sort(result.begin(), result.end());
    return result;
}
// std::vector<std::string> list_contracts;
// Хелпер: удалить свойство по строке
void delete_prop_str(JSContext* ctx, JSValueConst obj, const char* name) {
    JSAtom atom = JSI_NewAtom(ctx, name);
    if (atom != JSI_ATOM_NULL) {
        JSI_DeleteProperty(ctx, obj, atom, 0);
        JSI_FreeAtom(ctx, atom);
    }
}

// registerMethod(name, fn)
JSValue jsi_registerMethod(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]) || !JSI_IsFunction(ctx, argv[1])) {
        return JSI_ThrowTypeError(ctx, "registerMethod(name: string, fn: function)");
    }
    JSScope<20,20> scope(ctx);    
    auto  name = scope.toStdString(argv[0]);
    if (!name.size()) return JSI_EXCEPTION;

    // Сохраняем функцию (увеличиваем refcount)
    methods[name] = {ctx, JSI_DupValue(ctx, argv[1])};
    return JSI_UNDEFINED;
}

// registerView(name, fn)
JSValue jsi_registerView(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]) || !JSI_IsFunction(ctx, argv[1])) {
        return JSI_ThrowTypeError(ctx, "registerView(name: string, fn: function)");
    }
    JSScope<20,20> scope(ctx);    
    auto name = scope.toStdString(argv[0]);
    if (!name.size()) return JSI_EXCEPTION;

    views[name] = {ctx,JSI_DupValue(ctx, argv[1])};
    return JSI_UNDEFINED;
}

// setGranule(key, value)
JSValue jsi_setGranule(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0])) {
        return JSI_ThrowTypeError(ctx, "setGranule(key: string, value: any)");
    }
    JSScope<20,20> scope(ctx);    
    auto key = scope.toStdString(argv[0]);
    if (!key.size()) return JSI_EXCEPTION;

    // Сохраняем значение (увеличиваем refcount)
    granules[key] = {ctx,JSI_DupValue(ctx, argv[1])};
    return JSI_UNDEFINED;
}

// getGranule(key) -> value | undefined
JSValue jsi_getGranule(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1 || !JSI_IsString(argv[0])) {
        return JSI_ThrowTypeError(ctx, "getGranule(key: string)");
    }
    JSScope<20,20> scope(ctx);    
    auto key = scope.toStdString(argv[0]);
    if (!key.size()) return JSI_EXCEPTION;

    auto it = granules.find(key);
    JSValue result = (it != granules.end()) ? JSI_DupValue(ctx, it->second.get()) : JSI_UNDEFINED;
    return result;
}

// Регистрация API в глобальном объекте
void register_api(JSContext* ctx) {
    JSScope<20,20> scope(ctx);    
    JSValue global = JSI_GetGlobalObject(ctx);
    scope.addValue(global);

    JSI_SetPropertyStr(ctx, global, "registerMethod",
        JSI_NewCFunction(ctx, jsi_registerMethod, "registerMethod", 2));
    JSI_SetPropertyStr(ctx, global, "registerView",
        JSI_NewCFunction(ctx, jsi_registerView, "registerView", 2));
    JSI_SetPropertyStr(ctx, global, "setGranule",
        JSI_NewCFunction(ctx, jsi_setGranule, "setGranule", 2));
    JSI_SetPropertyStr(ctx, global, "getGranule",
        JSI_NewCFunction(ctx, jsi_getGranule, "getGranule", 1));
}
// Читает файл в строку
std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("cannot open file: " + path);
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Печатает исключение QuickJS
void print_exception(JSContext* ctx) {
    JSScope<20,20> scope(ctx);    

    JSValue exc = JSI_GetException(ctx);
    scope.addValue(exc);
    auto str = scope.toStdString(exc);
    if (str.size()) {
        std::cerr << "Exception: " << str << std::endl;
    }
}
// #include <quickjs.h>
// #include <string>
// #include <vector>
// #include <iostream>

struct Tx {
    std::string method;
    JSValue params;        // владеем ссылкой (JSI_DupValue)
    bool expect_error;
    JSValue expect_result; // JSI_UNDEFINED, если не задан
};
// Загружает массив транзакций из JS-файла.
// Возвращает true при успехе. Транзакции складываются в out.
// ВАЖНО: params и expect_result увеличивают refcount — их нужно
// освобождать через JSI_FreeValue после использования.
bool load_transactions(JSContext* ctx, const std::string& path, std::vector<Tx>& out) {
    // 1. Читаем файл
    std::string src;
    {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            std::cerr << "cannot open " << path << std::endl;
            return false;
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        src.resize(size);
        auto sz=fread(&src[0], 1, size, f);
        fclose(f);
        if(sz!=size)
            throw std::runtime_error("if(sz!=size)");
    }

    // 2. Компилируем
    JSValue compiled = JSI_Eval(
        ctx,
        src.data(),
        src.size(),
        path.c_str(),
        JSI_EVAL_FLAG_STRICT | JSI_EVAL_TYPE_GLOBAL | JSI_EVAL_FLAG_COMPILE_ONLY
    );

    if (JSI_IsException(compiled)) {
        JSValue exc = JSI_GetException(ctx);
        const char* str = JSI_ToCString(ctx, exc);
        if (str) {
            std::cerr << "compile error in " << path << ": " << str << std::endl;
            JSI_FreeCString(ctx, str);
        }
        JSI_FreeValue(ctx, exc);
        return false;
    }

    // 3. Выполняем — результат должен быть массивом
    JSValue result = JSI_EvalFunction(ctx, compiled);
    if (JSI_IsException(result)) {
        JSValue exc = JSI_GetException(ctx);
        const char* str = JSI_ToCString(ctx, exc);
        if (str) {
            std::cerr << "eval error in " << path << ": " << str << std::endl;
            JSI_FreeCString(ctx, str);
        }
        JSI_FreeValue(ctx, exc);
        return false;
    }

    if (!JSI_IsArray(ctx, result)) {
        std::cerr << path << ": result is not an array" << std::endl;
        JSI_FreeValue(ctx, result);
        return false;
    }

    // 4. Получаем длину массива
    JSValue length_val = JSI_GetPropertyStr(ctx, result, "length");
    int64_t length = 0;
    if (JSI_ToInt64(ctx, &length, length_val) != 0) {
        std::cerr << path << ": cannot read array length" << std::endl;
        JSI_FreeValue(ctx, length_val);
        JSI_FreeValue(ctx, result);
        return false;
    }
    JSI_FreeValue(ctx, length_val);

    // 5. Проходим по элементам
    for (int64_t i = 0; i < length; ++i) {
        JSValue item = JSI_GetPropertyUint32(ctx, result, (uint32_t)i);
        if (JSI_IsException(item) || !JSI_IsObject(item)) {
            std::cerr << path << ": tx[" << i << "] is not an object" << std::endl;
            JSI_FreeValue(ctx, item);
            JSI_FreeValue(ctx, result);
            return false;
        }

        Tx tx;
        tx.expect_error = false;
        tx.expect_result = JSI_UNDEFINED;

        // method
        JSValue method_val = JSI_GetPropertyStr(ctx, item, "method");
        if (!JSI_IsString(method_val)) {
            std::cerr << path << ": tx[" << i << "].method is not a string" << std::endl;
            JSI_FreeValue(ctx, method_val);
            JSI_FreeValue(ctx, item);
            JSI_FreeValue(ctx, result);
            return false;
        }
        const char* method_str = JSI_ToCString(ctx, method_val);
        if (!method_str) {
            JSI_FreeValue(ctx, method_val);
            JSI_FreeValue(ctx, item);
            JSI_FreeValue(ctx, result);
            return false;
        }
        tx.method = method_str;
        JSI_FreeCString(ctx, method_str);
        JSI_FreeValue(ctx, method_val);

        // params (владеем ссылкой)
        JSValue params_val = JSI_GetPropertyStr(ctx, item, "params");
        if (JSI_IsUndefined(params_val)) {
            // params не обязателен — пустой объект
            tx.params = JSI_NewObject(ctx);
        } else {
            tx.params = JSI_DupValue(ctx, params_val);
        }
        JSI_FreeValue(ctx, params_val);

        // expect_error
        JSValue expect_error_val = JSI_GetPropertyStr(ctx, item, "expect_error");
        if (!JSI_IsUndefined(expect_error_val)) {
            tx.expect_error = JSI_ToBool(ctx, expect_error_val);
        }
        JSI_FreeValue(ctx, expect_error_val);

        // expect_result
        JSValue expect_result_val = JSI_GetPropertyStr(ctx, item, "expect_result");
        if (!JSI_IsUndefined(expect_result_val)) {
            tx.expect_result = JSI_DupValue(ctx, expect_result_val);
        }
        JSI_FreeValue(ctx, expect_result_val);

        out.push_back(std::move(tx));
        JSI_FreeValue(ctx, item);
    }

    JSI_FreeValue(ctx, result);
    return true;
}
// Выполняет JS-файл как скрипт (не модуль)
bool run_script(JSContext* ctx, const std::string& path) {
    std::string src;
    try {
        src = read_file(path);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
    // printf("src %s",src.c_str());
    // Компиляция
    JSValue compiled = JSI_Eval(
        ctx,
        src.data(),
        src.size(),
        path.c_str(),
        JSI_EVAL_TYPE_GLOBAL | JSI_EVAL_FLAG_COMPILE_ONLY
    );

    if (JSI_IsException(compiled)) {
        print_exception(ctx);
        return false;
    }

    // Выполнение
    JSValue result = JSI_EvalFunction(ctx, compiled);
    if (JSI_IsException(result)) {
        print_exception(ctx);
        JSI_FreeValue(ctx, result);
        return false;
    }


    JSI_FreeValue(ctx, result);
    return true;
}
// Вспомогательная функция для удаления по строке
// void delete_prop_str(JSContext* ctx, JSValueConst obj, const char* name) {
//     JSAtom atom = JSI_NewAtom(ctx, name);
//     if (atom != JSI_ATOM_NULL) {
//         JSI_DeleteProperty(ctx, obj, atom, 0);
//         JSI_FreeAtom(ctx, atom);
//     }
// }

// Использование
// delete_prop_str(ctx, global, "Math");
// delete_prop_str(ctx, global, "Date");
// delete_prop_str(ctx, global, "RegExp");
// и т.д.

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.js> [file2.js ...]" << std::endl;
        return 1;
    }

    // Создаём runtime и контекст
    JSRuntime* rt = JSI_NewRuntime();
    if (!rt) {
        std::cerr << "cannot create runtime" << std::endl;
        return 1;
    }

    // Ограничения (опционально)
    JSI_SetMemoryLimit(rt, 64 * 1024 * 1024);      // 64 MB
    JSI_SetMaxStackSize(rt, 1024 * 1024);          // 1 MB

    JSContext* ctx = JSI_NewContext(rt);
    if (!ctx) {
        std::cerr << "cannot create context" << std::endl;
        JSI_FreeRuntime(rt);
        return 1;
    }
    register_api(ctx);
    // Удаляем опасные глобальные объекты (пример)
// #ifdef KALL
    JSValue global = JSI_GetGlobalObject(ctx);
    delete_prop_str(ctx, global, "Math");
    delete_prop_str(ctx, global, "Date");
    delete_prop_str(ctx, global, "RegExp");
    delete_prop_str(ctx, global, "JSON");
    delete_prop_str(ctx, global, "Proxy");
    delete_prop_str(ctx, global, "Promise");
    delete_prop_str(ctx, global, "Map");
    delete_prop_str(ctx, global, "Set");
    delete_prop_str(ctx, global, "WeakRef");
    delete_prop_str(ctx, global, "FinalizationRegistry");
    delete_prop_str(ctx, global, "Atomics");
    delete_prop_str(ctx, global, "SharedArrayBuffer");
    JSI_FreeValue(ctx, global);
// #endif
    int failures = 0;

    std::vector<Tx> tx;
    load_transactions(ctx,"tx",tx);

    auto dir=list_directory("tx");



    // Выполняем каждый файл в новом контексте? 
    // Здесь — в одном контексте, последовательно.
    for (auto &z: dir) {
        std::cout << "Running: " << z << std::endl;
        if (!run_script(ctx, z)) {
            failures++;
        }
    }

// for (auto& [k, v] : granules) JSI_FreeValue(ctx, v);
granules.clear();
// for (auto& [k, v] : methods) JSI_FreeValue(ctx, v);
methods.clear();
// for (auto& [k, v] : views) JSI_FreeValue(ctx, v);
views.clear();

// JSI_FreeContext(ctx);
// JSI_FreeRuntime(rt);
    // Освобождаем
    JSI_FreeContext(ctx);
    JSI_FreeRuntime(rt);

    if (failures > 0) {
        std::cerr << failures << " file(s) failed" << std::endl;
        return 1;
    }

    std::cout << "All files executed successfully" << std::endl;
    return 0;
}