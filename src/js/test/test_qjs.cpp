#include <quickjs/quickjs.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <iomanip>
#include <memory>

class QuickJSTest {
private:
    JSRuntime* runtime;
    
public:
    QuickJSTest() {
        // Создаём один runtime для всех тестов
        runtime = JS_NewRuntime();
        if (!runtime) {
            throw std::runtime_error("Failed to create JSRuntime");
        }
    }
    
    ~QuickJSTest() {
        if (runtime) {
            JS_FreeRuntime(runtime);
        }
    }
    
    // Создание контекста и выполнение JS-кода
    std::pair<double, bool> createContextAndExecute(const std::string& jsCode, int contextId) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Создаём контекст
        JSContext* ctx = JS_NewContext(runtime);
        if (!ctx) {
            return {0.0, false};
        }
        
        // Выполняем JS-код
        const char* code = jsCode.c_str();
        JSValue result = JS_Eval(ctx, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
        
        bool success = !JS_IsException(result);
        
        // Очищаем ресурсы
        JS_FreeValue(ctx, result);
        JS_FreeContext(ctx);
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::micro>(end - start).count();
        
        return {duration, success};
    }
    
    // Запуск теста с множеством итераций
    void runTest(int iterations, const std::string& jsCode) {
        std::cout << "\n🚀 Запуск теста: " << iterations << " итераций\n";
        std::cout << "📝 JS код: \"" << jsCode << "\"\n\n";
        
        std::vector<double> times;
        int successCount = 0;
        int failCount = 0;
        
        auto totalStart = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            auto [duration, success] = createContextAndExecute(jsCode, i);
            if (success) {
                times.push_back(duration);
                successCount++;
            } else {
                failCount++;
            }
        }
        
        auto totalEnd = std::chrono::high_resolution_clock::now();
        double totalTime = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();
        
        // Статистика
        if (!times.empty()) {
            double sum = 0;
            double minTime = times[0];
            double maxTime = times[0];
            
            for (double t : times) {
                sum += t;
                if (t < minTime) minTime = t;
                if (t > maxTime) maxTime = t;
            }
            
            double avgTime = sum / times.size();
            
            std::cout << "📊 Результаты теста:\n";
            std::cout << "✅ Успешно: " << successCount << "\n";
            std::cout << "❌ Ошибок: " << failCount << "\n";
            std::cout << "⏱️  Общее время: " << std::fixed << std::setprecision(2) << totalTime << "ms\n";
            std::cout << "⚡ Среднее время на контекст: " << std::setprecision(3) << avgTime << "μs\n";
            std::cout << "🐌 Минимальное время: " << minTime << "μs\n";
            std::cout << "🚀 Максимальное время: " << maxTime << "μs\n";
            std::cout << "📈 Контекстов в секунду: " << (iterations / (totalTime / 1000.0)) << "\n";
        }
    }
    
    // Сравнение разных типов JS-кода
    void compareDifferentCode() {
        std::cout << "\n🔬 СРАВНЕНИЕ РАЗНЫХ ТИПОВ JS КОДА\n";
        std::cout << std::string(50, '=') << "\n";
        
        struct TestCase {
            std::string name;
            std::string code;
        };
        
        std::vector<TestCase> tests = {
            {"Простая математика", "2 + 2;"},
            {"Работа со строками", "'Hello ' + 'World';"},
            {"Цикл 1000 итераций", "let sum = 0; for(let i = 0; i < 1000; i++) sum += i; sum;"},
            {"Создание объекта", "({a: 1, b: 2, c: 3}).c;"},
            {"Массив и reduce", "[1,2,3,4,5].reduce((a,b)=>a+b,0);"},
            {"Math.random", "Math.random() * 100;"}
        };
        
        int iterations = 100;
        
        for (const auto& test : tests) {
            std::cout << "\n📋 Тест: " << test.name << "\n";
            runTest(iterations, test.code);
            std::cout << std::string(50, '-') << "\n";
        }
    }
    
    // Тест масштабирования
    void testScaling() {
        std::cout << "\n📈 ТЕСТ МАСШТАБИРОВАНИЯ\n";
        std::cout << std::string(50, '=') << "\n";
        
        std::vector<int> iterationsList = {10, 50, 100, 500, 1000, 5000};
        std::string jsCode = "Math.random() * 100;";
        
        std::cout << "\n┌─────────────┬──────────────┬──────────────────┐\n";
        std::cout << "│ Контекстов  │ Общее время  │ Контекстов/сек   │\n";
        std::cout << "├─────────────┼──────────────┼──────────────────┤\n";
        
        for (int iterations : iterationsList) {
            auto totalStart = std::chrono::high_resolution_clock::now();
            
            int successCount = 0;
            for (int i = 0; i < iterations; i++) {
                JSContext* ctx = JS_NewContext(runtime);
                if (ctx) {
                    JSValue result = JS_Eval(ctx, jsCode.c_str(), jsCode.length(), "<test>", JS_EVAL_TYPE_GLOBAL);
                    if (!JS_IsException(result)) {
                        successCount++;
                    }
                    JS_FreeValue(ctx, result);
                    JS_FreeContext(ctx);
                }
            }
            
            auto totalEnd = std::chrono::high_resolution_clock::now();
            double totalTime = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();
            double contextsPerSec = (successCount / (totalTime / 1000.0));
            
            std::cout << "│ " << std::setw(11) << iterations << " │ "
                      << std::setw(12) << std::fixed << std::setprecision(2) << totalTime << "ms │ "
                      << std::setw(16) << std::fixed << std::setprecision(0) << contextsPerSec << " │\n";
        }
        
        std::cout << "└─────────────┴──────────────┴──────────────────┘\n";
    }
};

// Бенчмарк производительности C-функций vs JS-функций
void benchmarkCFunctionsVsJS() {
    std::cout << "\n⚡ БЕНЧМАРК: C-функции vs JS-функции\n";
    std::cout << std::string(50, '=') << "\n";
    
    JSRuntime* rt = JS_NewRuntime();
    JSContext* ctx = JS_NewContext(rt);
    
    // Регистрируем C-функцию, которая возвращает 6
    JSValue nativeFunc = JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, 
                                                  int argc, JSValueConst* argv) {
        return JS_NewInt32(ctx, 6);
    }, "nativeFunc", 0);
    
    JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "nativeFunc", nativeFunc);
    
    // JS-функция, которая возвращает 6
    JS_Eval(ctx, "function jsFunc() { return 6; }", strlen("function jsFunc() { return 6; }"), 
            "<test>", JS_EVAL_TYPE_GLOBAL);
    
    const int iterations = 1000000;
    
    // Тест C-функции
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        JSValue result = JS_Eval(ctx, "nativeFunc();", strlen("nativeFunc();"), 
                                  "<test>", JS_EVAL_TYPE_GLOBAL);
        JS_FreeValue(ctx, result);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double cTime = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Тест JS-функции
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i <iterations; i++) {
        JSValue result = JS_Eval(ctx, "jsFunc();", strlen("jsFunc();"), 
                                  "<test>", JS_EVAL_TYPE_GLOBAL);
        JS_FreeValue(ctx, result);
    }
    end = std::chrono::high_resolution_clock::now();
    double jsTime = std::chrono::duration<double, std::milli>(end - start).count();
    
    std::cout << "\nC-функция:    " << std::fixed << std::setprecision(2) << cTime << "ms\n";
    std::cout << "JS-функция:   " << jsTime << "ms\n";
    std::cout << "Разница:      C-функция в " << (jsTime / cTime) << "x быстрее\n";
    
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

int main() {
    std::cout << "⚡ ТЕСТ СКОРОСТИ СОЗДАНИЯ КОНТЕКСТОВ QUICKJS\n";
    std::cout << std::string(50, '=') << "\n";
    
    try {
        QuickJSTest tester;
        
        // Базовый тест
        tester.runTest(1000, "2 + 2;");
        
        // Сравнение разных типов кода
        tester.compareDifferentCode();
        
        // Тест масштабирования
        tester.testScaling();
        
        // Бенчмарк C-функций
        benchmarkCFunctionsVsJS();
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
