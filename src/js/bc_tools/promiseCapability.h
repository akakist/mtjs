#pragma once
#include "quickjspp.hpp"

struct PromiseCapability {
    qjs::Value promise;
    qjs::Value resolve;
    qjs::Value reject;

    PromiseCapability(qjs::Context &ctx) {
        JSValue funcs[2];
        JSValue p = JS_NewPromiseCapability(ctx, funcs);

        promise = qjs::Value(ctx, p);
        resolve = qjs::Value(ctx, funcs[0]);
        reject  = qjs::Value(ctx, funcs[1]);

        // освобождение исходных JSValue, так как мы их обернули в qjs::Value
        JS_FreeValue(ctx, p);
        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
    }
};
