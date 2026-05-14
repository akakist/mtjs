// js_value_guard.h
#pragma once
#include <quickjs.h>

class JSValueGuard {
    JSContext* ctx = nullptr;
    JSValue val = JS_UNDEFINED;

public:
    // Конструкторы
    JSValueGuard() = default;

    JSValueGuard(JSContext* c, JSValue v) : ctx(c), val(v) {}

    // Конструктор копирования (увеличивает счётчик)
    JSValueGuard(const JSValueGuard& other)
        : ctx(other.ctx), val(JS_DupValue(other.ctx, other.val)) {}

    // Оператор присваивания (копирование)
    JSValueGuard& operator=(const JSValueGuard& other) {
        if (this != &other) {
            // Освобождаем текущее значение
            if (ctx && !JS_IsUndefined(val) && !JS_IsNull(val)) {
                JS_FreeValue(ctx, val);
            }
            // Копируем новое
            ctx = other.ctx;
            val = JS_DupValue(other.ctx, other.val);
        }
        return *this;
    }

    // Деструктор
    ~JSValueGuard() {
        if (ctx && !JS_IsUndefined(val) && !JS_IsNull(val)) {
            JS_FreeValue(ctx, val);
        }
    }

    // Доступ к значению
    JSValue get() const {
        return val;
    }
    JSContext* context() const {
        return ctx;
    }

    // Проверка
    explicit operator bool() const {
        return ctx && !JS_IsUndefined(val) && !JS_IsNull(val);
    }

    // Отдать владение (не освобождать при разрушении)
    JSValue release() {
        JSValue tmp = val;
        val = JS_UNDEFINED;
        ctx = nullptr;
        return tmp;
    }

    // Очистить
    void clear() {
        if (ctx && !JS_IsUndefined(val) && !JS_IsNull(val)) {
            JS_FreeValue(ctx, val);
        }
        ctx = nullptr;
        val = JS_UNDEFINED;
    }
};
