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
#include "md/md_GetUserStatusREQ.h"
#include "js_tools.h"
// #include ""

static std::string js_obj_to_kv(JSContext *ctx,
                                JSValueConst obj)
{

    JSScope<100, 100> scope(ctx);
    JSPropertyEnum *props;
    uint32_t len;
    if (JS_GetOwnPropertyNames(ctx, &props, &len, obj,
                               JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0)
    {
        throw CommonError("JS_GetOwnPropertyNames(ctx, &props, &len, obj,");
    }

    std::string result;

    for (uint32_t i = 0; i < len; i++)
    {
        const char *key = JS_AtomToCString(ctx, props[i].atom);
        JSValue val = JS_GetProperty(ctx, obj, props[i].atom);
        const char *val_str = JS_ToCString(ctx, val);

        if (key && val_str)
        {
            result += key;
            result += "=";
            result += val_str;
            if (i + 1 < len)
            {
                result += "\n";
            }
        }

        JS_FreeCString(ctx, key);
        JS_FreeCString(ctx, val_str);
        JS_FreeValue(ctx, val);
    }

    js_free(ctx, props);

    return result;
}

JSValue js_add_instance(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;

    try
    {
        JSScope<10, 10> scope(ctx);
        mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
        if (!op)
            return JS_ThrowInternalError(ctx, "!op");
        op->rpcBlockExit = true;

        if (argc != 2)
            return JS_ThrowInternalError(ctx, "number of argument must be 2");

        std::string iname = (std::string)scope.toStdStringView(argv[0]);

        std::string cnf = (std::string)scope.toStdStringView(argv[1]);

        // if(!JS_IsObject(argv[0]))
        //     return JS_ThrowTypeError(ctx, "if(!JS_IsObject(argv[0]))");

        IInstance *instance1 = iUtils->createNewInstance(iname);
        ConfigObj *cnf1 = new ConfigObj(iname, cnf);
        instance1->setConfig(cnf1);
        instance1->initServices();

        return JS_UNDEFINED;
    }
    catch (std::exception &e)
    {
        logErr2("exception %s", e.what());
        return JS_ThrowInternalError(ctx, "exception %s", e.what());
    }
    return JS_ThrowInternalError(ctx, "return values wrong");
}
JSValue js_tx_subscribe(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "!op");
    if (argc != 2)
        return JS_ThrowInternalError(ctx, "number of argument must be 2");
    if (!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "node addr not specified");
    if (!JS_IsFunction(ctx, argv[1]))
        return JS_ThrowInternalError(ctx, "callback not specified");
    std::string node_addr = (std::string)scope.toStdStringView(argv[0]);
    // JSValue cb = argv[1];
    op->tx_subscription_cb.emplace(JSValueGuard(ctx, JS_DupValue(ctx, argv[1])));

    op->broadcaster->sendEvent(node_addr, ServiceEnum::BlockStreamer, new bcEvent::ClientTxSubscribeREQ(op->listener_->serviceId));

    return JS_UNDEFINED;
}
////////////////
JSValue js_tx_submit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    logErr2("js_tx_submit");
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (argc != 5)
        return JS_ThrowInternalError(ctx, "number of argument must be 5");

    if (!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "node addr not specified, must be string");
    if (!JS_IsNumber(argv[1]))
        return JS_ThrowInternalError(ctx, "timeout not specified, must be number (float)");
    if (!JS_IsString(argv[2]))
        return JS_ThrowInternalError(ctx, "msg not specified, must be string");
    if (!JS_IsString(argv[3]))
        return JS_ThrowInternalError(ctx, "sk not specified, must be string");
    if (!JS_IsString(argv[4]))
        return JS_ThrowInternalError(ctx, "nonce not specified, must be string");

    auto node_addr = scope.toStdString(argv[0]);
    // logErr2("node_addr %s", node_addr.c_str());
    double timeout;
    if (JS_ToFloat64(ctx, &timeout, argv[1]))
    {
        return JS_ThrowInternalError(ctx, "error parsing timeout");
    }
    auto msg = scope.toStdString(argv[2]);
    auto sk = base16::decode(scope.toStdString(argv[3]));
    auto pk = extract_public_ed(sk);
    auto nonce_str = scope.toStdString(argv[4]);
    uint64_t nonce=0;
    try
    {        nonce=std::atoll(nonce_str.c_str());
    }    
    catch (std::exception &e)
    {
                return JS_ThrowInternalError(ctx, "error parsing nonce: %s", e.what());
    }

    // auto hash=blake2b_hash(msg);

    REF_getter<MsgData::TX> t = new MsgData::TX;
    t->tx_body = msg;
    t->pk_ed_bin = pk;
    t->nonce = nonce;
    auto hash = t->getHash();
    t->sig_ed_bin = sign_ed(sk, hash.container);
    // auto hash = blake2b_hash(t->tx_body+t->nonce.toString());
    // logErr2("op->broadcaster->sendEvent(node_addr, ServiceEnum::TxValidator, new bcEvent::AddTxREQ(t, op->listener_->serviceId));");
    op->broadcaster->sendEvent(node_addr, ServiceEnum::TxValidator, new bcEvent::AddTxREQ(t, op->listener_->serviceId));

    op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, timeout, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise))
    {
        return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    }

    pd.resolve = g_resolve;
    pd.reject = g_reject;

    return g_prom.release();
}
JSValue js_addr_from_pk(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    // mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (argc != 1)
        return JS_ThrowInternalError(ctx, "number of argument must be 1");
    if (!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "pk not specified, must be string");
    auto pk = base16::decode(scope.toStdString(argv[0]));
    auto addr = blake2b_hash(pk);
    auto addr_str = base16::encode(addr.container);
    logErr2("addr from pk: %s", addr_str.c_str());
    return JS_NewString(ctx, addr_str.c_str());
}
/////////////////
JSValue js_get_user_info(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (argc != 3)
        return JS_ThrowInternalError(ctx, "number of argument must be 3");

    if (!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "node addr not specified");
    if (!JS_IsString(argv[1]))
        return JS_ThrowInternalError(ctx, "nick not specified");
    if (!JS_IsNumber(argv[2]))
        return JS_ThrowInternalError(ctx, "timeout not specified");

    std::string node_addr = (std::string)scope.toStdString(argv[0]);
    ADDRESS_id address;
    address.addr = base16::decode(scope.toStdString(argv[1]));
    // std::string nick=(std::string)scope.toStdString(argv[1]);
    double to;
    if (JS_ToFloat64(ctx, &to, argv[2]))
        return JS_ThrowInternalError(ctx, "timeout parse error");

    REF_getter<MsgData::GetUserStatusREQ> rq = new MsgData::GetUserStatusREQ();
    rq->user_address = address;
    rq->rnd.resize(10);
    RAND_bytes((unsigned char *)rq->rnd.data(), rq->rnd.size());

    auto hash = rq->getHash();
    op->broadcaster->sendEvent(scope.toStdString(argv[0]), ServiceEnum::GrainReader, new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));

    op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;

    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise))
    {
        return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    }
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}

void js_register_add_instance(JSContext *ctx, JSValue &mtjs_obj)
{
    MUTEX_INSPECTOR;
    logErr2("js_register_add_instance");
    JS_SetPropertyStr(ctx, mtjs_obj, "addInstance", JS_NewCFunction(ctx, js_add_instance, "addInstance", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "tx_submit", JS_NewCFunction(ctx, js_tx_submit, "tx_submit", 2));
    // JS_SetPropertyStr(ctx, mtjs_obj, "tx_sign", JS_NewCFunction(ctx, js_tx_sign, "tx_sign", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "tx_subscribe", JS_NewCFunction(ctx, js_tx_subscribe, "tx_subscribe", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_user_info", JS_NewCFunction(ctx, js_get_user_info, "get_user_info", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "addr_from_pk", JS_NewCFunction(ctx, js_addr_from_pk, "addr_from_pk", 1));
}
