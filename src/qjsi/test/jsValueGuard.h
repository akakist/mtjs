// jsi_value_guard.h
#pragma once
#include <quickjs_i.h>

class JSIValueGuard {
    JSIContext* ctx = nullptr;
    JSIValue val = JSI_UNDEFINED;

public:
    // Конструкторы
    JSIValueGuard() = default;

    JSIValueGuard(JSIContext* c, JSIValue v) : ctx(c), val(v) {}

    // Конструктор копирования (увеличивает счётчик)
    JSIValueGuard(const JSIValueGuard& other)
        : ctx(other.ctx), val(JSI_DupValue(other.ctx, other.val)) {}

    // Оператор присваивания (копирование)
    JSIValueGuard& operator=(const JSIValueGuard& other) {
        if (this != &other) {
            // Освобождаем текущее значение
            if (ctx && !JSI_IsUndefined(val) && !JSI_IsNull(val)) {
                JSI_FreeValue(ctx, val);
            }
            // Копируем новое
            ctx = other.ctx;
            val = JSI_DupValue(other.ctx, other.val);
        }
        return *this;
    }

    // Деструктор
    ~JSIValueGuard() {
        if (ctx && !JSI_IsUndefined(val) && !JSI_IsNull(val)) {
            JSI_FreeValue(ctx, val);
        }
    }

    // Доступ к значению
    JSIValue get() const {
        return val;
    }
    JSIContext* context() const {
        return ctx;
    }

    // Проверка
    explicit operator bool() const {
        return ctx && !JSI_IsUndefined(val) && !JSI_IsNull(val);
    }

    // Отдать владение (не освобождать при разрушении)
    JSIValue release() {
        JSIValue tmp = val;
        val = JSI_UNDEFINED;
        ctx = nullptr;
        return tmp;
    }

    // Очистить
    void clear() {
        if (ctx && !JSI_IsUndefined(val) && !JSI_IsNull(val)) {
            JSI_FreeValue(ctx, val);
        }
        ctx = nullptr;
        val = JSI_UNDEFINED;
    }
};

inline int operator< (const JSIValueGuard &a, const JSIValueGuard &b)
{
    if(a.get().tag != b.get().tag) return a.get().tag < b.get().tag;
    if(a.get().u.ptr != b.get().u.ptr) return  a.get().u.ptr < b.get().u.ptr;
    return false;
}
