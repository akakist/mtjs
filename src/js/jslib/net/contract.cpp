#include <quickjs.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "REF.h"
#include "blake2bHasher.h"
#include "common/async_task.h"
#include "common/jsscope.h"
#include "common/mtjs_opaque.h"
#include "main/configObj.h"
#include "bcEvent.h"

#include "md_TX.h"
#include "quickjs.h"

#include "quickjs.h"
#include <string.h>
#include <stdio.h>
#include <openssl/rand.h>

#include "quickjs.h"
#include <string>
#include <vector>
#include <cstdio>
#include "msg.h"
#include "Events/System/timerEvent.h"
#include "timers.h"
#include "jsValueGuard.h"
#include "md/md_GetUserNonceREQ.h"
#include "js_tools.h"
// #include ""
#include "contract_rt.h"

JSValue setupTxParams(JSContext* ctx, const std::string & senderHex, const std::string& txHashHex, uint64_t nonce,
uint64_t blockTimestamp,uint64_t blockHeight, const std::string& contract_address, const std::string& owner_hex)
{
    JSValue ret=JS_NewObject(ctx);
    if (JS_IsException(ret)) {
        return JS_ThrowTypeError(ctx, "Failed to create tx params object");
    }
    JS_SetPropertyStr(ctx,ret,"txSender",JS_NewStringLen(ctx,senderHex.data(),senderHex.size()));
    JS_SetPropertyStr(ctx,ret,"txHash",JS_NewStringLen(ctx,txHashHex.data(),txHashHex.size()));
    JS_SetPropertyStr(ctx, ret, "txNonce", JS_NewBigUint64(ctx, nonce));
    JS_SetPropertyStr(ctx, ret, "txTimestamp", JS_NewBigUint64(ctx, blockTimestamp));
    JS_SetPropertyStr(ctx, ret, "txBlockHeight", JS_NewBigUint64(ctx, blockHeight));
    JS_SetPropertyStr(ctx, ret, "txEpoch", JS_NewBigUint64(ctx, blockHeight));
    JS_SetPropertyStr(ctx, ret, "contractAddress", JS_NewStringLen(ctx, contract_address.data(), contract_address.size()));
    JS_SetPropertyStr(ctx, ret, "ownerAddress", JS_NewStringLen(ctx, owner_hex.data(), owner_hex.size()));

    return ret;

}
JSValue js_txHash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    execute_context *op = (execute_context *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 0)
        return JS_ThrowInternalError(ctx, "number of argument must be 0");
    return JS_NewArrayBufferCopy(ctx, (uint8_t*)op->tx_hash_bin.data(), op->tx_hash_bin.size());
}
JSValue js_txSender(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    execute_context *op = (execute_context *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 0)
        return JS_ThrowInternalError(ctx, "number of argument must be 0");
    return JS_NewArrayBufferCopy(ctx, (uint8_t*)op->tx_sender_bin.data(), op->tx_sender_bin.size());
}
JSValue js_txNonce(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    execute_context *op = (execute_context *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 0)
        return JS_ThrowInternalError(ctx, "number of argument must be 0");
    return JS_NewBigUint64(ctx, op->nonce);
}
JSValue js_txEpoch(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    execute_context *op = (execute_context *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 0)
        return JS_ThrowInternalError(ctx, "number of argument must be 0");
    return JS_NewBigUint64(ctx, op->txEpoch);
}
JSValue js_contractOwner(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    execute_context *op = (execute_context *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 0)
        return JS_ThrowInternalError(ctx, "number of argument must be 0");
    return JS_NewArrayBufferCopy(ctx, (uint8_t*)op->ownerAddress.data(), op->ownerAddress.size());
    // return JS_NewBigUint64(ctx, op->txEpoch);
}
JSValue js_contractAddress(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    execute_context *op = (execute_context *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 0)
        return JS_ThrowInternalError(ctx, "number of argument must be 0");
    return JS_NewArrayBufferCopy(ctx, (uint8_t*)op->ownerAddress.data(), op->ownerAddress.size());
    // return JS_NewBigUint64(ctx, op->txEpoch);
}

JSValue js_register_mutable_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSScope<10, 10> scope(ctx);
    execute_context *op = (execute_context *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 2)
        return JS_ThrowInternalError(ctx, "number of argument must be 2");
    if (!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "method name not specified");
    if (!JS_IsFunction(ctx, argv[1]))
        return JS_ThrowInternalError(ctx, "callback not specified");
    std::string method_name = (std::string)scope.toStdStringView(argv[0]);
    op->contract->mutable_methods.insert_or_assign(scope.toStdString(argv[0]),JSValueGuard(ctx, JS_DupValue(ctx,argv[1])));

    return JS_UNDEFINED;
}
JSValue js_register_view(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSScope<10, 10> scope(ctx);
    execute_context *op = (execute_context *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 2)
        return JS_ThrowInternalError(ctx, "number of argument must be 2");
    if (!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "method name not specified");
    if (!JS_IsFunction(ctx, argv[1]))
        return JS_ThrowInternalError(ctx, "callback not specified");
    std::string method_name = (std::string)scope.toStdStringView(argv[0]);
    op->contract->immutable_methods.insert_or_assign(scope.toStdString(argv[0]),JSValueGuard(ctx, JS_DupValue(ctx,argv[1])));
    return JS_UNDEFINED;
}

JSValue js_Call(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSScope<10, 10> scope(ctx);
    execute_context *op = (execute_context *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 3)
        return JS_ThrowInternalError(ctx, "number of argument must be 3");
    if (!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "contract name not specified");
    if (!JS_IsString(argv[1]))
        return JS_ThrowInternalError(ctx, "method name not specified");
    if (!JS_IsFunction(ctx, argv[1]))
        return JS_ThrowInternalError(ctx, "callback not specified");
    std::string method_name = (std::string)scope.toStdStringView(argv[0]);
    // op->immutable_methods.insert_or_assign(scope.toStdString(argv[0]),JSValueGuard(ctx, JS_DupValue(ctx,argv[1])));
    return JS_UNDEFINED;
}


////////////////

/////////////////

void js_register_contract(JSContext *ctx, JSValue &contract_obj)
{
    MUTEX_INSPECTOR;
    logErr2("js_register_contract");
    JS_SetPropertyStr(ctx, contract_obj, "registerMethod", JS_NewCFunction(ctx, js_register_mutable_method, "registerMethod", 2));
    JS_SetPropertyStr(ctx, contract_obj, "registerView", JS_NewCFunction(ctx, js_register_view, "registerView", 2));
    JS_SetPropertyStr(ctx, contract_obj, "txHash", JS_NewCFunction(ctx, js_txHash, "txHash", 0));
    JS_SetPropertyStr(ctx, contract_obj, "txSender", JS_NewCFunction(ctx, js_txSender, "txSender", 0));
    JS_SetPropertyStr(ctx, contract_obj, "txNonce", JS_NewCFunction(ctx, js_txNonce, "txNonce", 0));
    JS_SetPropertyStr(ctx, contract_obj, "txEpoch", JS_NewCFunction(ctx, js_txEpoch, "txEpoch", 0));
    JS_SetPropertyStr(ctx, contract_obj, "txBlockHeight", JS_NewCFunction(ctx, js_txEpoch, "txBlockHeight", 0));
    JS_SetPropertyStr(ctx, contract_obj, "contractOwner", JS_NewCFunction(ctx, js_contractOwner, "contractOwner", 0));


}
