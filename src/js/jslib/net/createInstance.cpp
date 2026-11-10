#define OPENSSL_SUPPRESS_DEPRECATED
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
#include "md/md_GetContractDataREQ.h"
#include "md/md_GetNodeInfoREQ.h"
#include "md/md_GetValuesREQ.h"
#include "md/md_GetContractREQ.h"
#include "md/md_CallViewREQ.h"
#include "md/md_CallViewRSP.h"
#include "js_tools.h"
#include "md/md_GetStateREQ.h"
#include "md/md_GetAddressInfoREQ.h"
#include "md/md_GetAddressInfoRSP.h"
#include "md/md_GetGranuleREQ.h"
#include "md/md_GetGranuleRSP.h"
#include "md/md_ScanREQ.h"
#include "md/md_ScanRSP.h"
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
JSValue js_blake2b_hex(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
{
    MUTEX_INSPECTOR;
    JSScope<10,10> scope(ctx);
    if (argc != 1 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "blake2b_hex(str)");
    std::string s = scope.toStdString(argv[0]);
    auto h = blake2b_hash(s);
    auto hex = base16::encode(h.container);
    return JS_NewStringLen(ctx, hex.data(), hex.size());
}

JSValue js_sign_ed(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
{
    MUTEX_INSPECTOR;
    JSScope<10,10> scope(ctx);
    if (argc != 2 || !JS_IsString(argv[0]) || !JS_IsString(argv[1]))
        return JS_ThrowTypeError(ctx, "sign_ed(sk_hex, msg)");
    std::string sk_hex = scope.toStdString(argv[0]);
    std::string msg    = scope.toStdString(argv[1]);
    std::string sk;
    try { sk = base16::decode(sk_hex); }
    catch (const std::exception& e) {
        return JS_ThrowTypeError(ctx, "sign_ed: bad sk hex: %s", e.what());
    }
    if (sk.size() != crypto_sign_SECRETKEYBYTES)
        return JS_ThrowTypeError(ctx, "sign_ed: sk must be %d bytes (got %zu)",
                                 crypto_sign_SECRETKEYBYTES, sk.size());
    auto sig = sign_ed(sk, msg);
    auto hex = base16::encode(sig);
    return JS_NewStringLen(ctx, hex.data(), hex.size());
}
JSValue js_get_contract(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "get_contract: !op");
    if (argc != 3) return JS_ThrowInternalError(ctx, "get_contract(node, contract, timeout)");
    if (!JS_IsString(argv[0])) return JS_ThrowInternalError(ctx, "get_contract: node must be string");
    if (!JS_IsString(argv[1])) return JS_ThrowInternalError(ctx, "get_contract: contract must be string");
    if (!JS_IsNumber(argv[2])) return JS_ThrowInternalError(ctx, "get_contract: timeout must be number");
    std::string node_addr = scope.toStdString(argv[0]);
    std::string contract = scope.toStdString(argv[1]);
    double to = 0;
    if (JS_ToFloat64(ctx, &to, argv[2])) return JS_ThrowInternalError(ctx, "get_contract: timeout parse error");

    REF_getter<MsgData::GetContractREQ> rq = new MsgData::GetContractREQ();
    rq->contract.container = contract;
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(node_addr, ServiceEnum::GrainReader,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
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

        IInstance *instance1 = iUtils->createNewInstance(iname);
        ConfigObj *cnf1 = new ConfigObj(iname, cnf);
        instance1->setConfig(cnf1);
        instance1->initServices();

        // handle — hex-строка (без потери точности через double)
        char buf[32];
        snprintf(buf, sizeof(buf), "%llx", (unsigned long long)(uintptr_t)instance1);
        return JS_NewString(ctx, buf);
    }
    catch (std::exception &e)
    {
        logErr2("exception %s", e.what());
        return JS_ThrowInternalError(ctx, "exception %s", e.what());
    }
    return JS_ThrowInternalError(ctx, "return values wrong");
}
JSValue js_kill_instance(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSScope<10,10> scope(ctx);
    if (argc != 1 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "killInstance(handle)");
    std::string hex = scope.toStdString(argv[0]);
    if (hex.empty())
        return JS_ThrowTypeError(ctx, "killInstance: empty handle");
    uintptr_t h = (uintptr_t)strtoull(hex.c_str(), NULL, 16);
    IInstance* instance1 = (IInstance*)h;
    if (!instance1)
        return JS_ThrowTypeError(ctx, "killInstance: null handle");
    iUtils->unregisterInstance(instance1);   // delete i → ~CInstance → deinitServices (stop+join+delete)
    return JS_UNDEFINED;
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
    // logErr2("js_tx_submit");
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (argc != 6)
        return JS_ThrowInternalError(ctx, "number of argument must be 6");

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
    if (!JS_IsFunction(ctx, argv[5]))
        return JS_ThrowInternalError(ctx, "callback not specified, must be function");

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
    // JSValueGuard g_func(ctx,JS_DupValue(ctx,argv[5]));
    int64_t nonce=0;
    try
    {   nonce=std::atoll(nonce_str.c_str());
    }
    catch (std::exception &e)
    {
        return JS_ThrowInternalError(ctx, "error parsing nonce: %s", e.what());
    }


    REF_getter<MsgData::TX> t = new MsgData::TX;
    t->tx_body = msg;
    t->pk_ed_bin = pk;
    // t->nonce = nonce;
    auto hash = t->getHash();
    t->sig_ed_bin = sign_ed(sk, hash.container);
    op->broadcaster->sendEvent(node_addr, ServiceEnum::TxValidator, new bcEvent::AddTxREQ(t, op->listener_->serviceId));
    if (JS_IsFunction(ctx, argv[5])) {
        op->node_tx_cb.emplace(hash.container, JSValueGuard(ctx, JS_DupValue(ctx, argv[5])));
        // подписка на блок с этой TX (BlockStreamer шлёт RSP только если tx_hash в блоке)
        THASH_id h;
        h.container = hash.container;
        op->broadcaster->sendEvent(node_addr, ServiceEnum::BlockStreamer,
                                   new bcEvent::ClientTxWaitREQ(h, (time_t)timeout, op->listener_->serviceId));
        // таймер на блок (TIMER_TX_WAIT_BLOCK)
        op->broadcaster->sendEvent(ServiceEnum::Timer,
                                   new timerEvent::SetAlarm(Timers::TIMER_TX_WAIT_BLOCK, toRef(hash.container), NULL, timeout, op->listener_));
    }

    op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, timeout, op->listener_));

    // op->node_tx_cb.emplace(hash.container,JSValueGuard(ctx,JS_DupValue(ctx,argv[5])));
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
/////////////////

JSValue js_get_address_info(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    JSScope<10,10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque*)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "get_address_info: !op");
    if (argc != 3) return JS_ThrowInternalError(ctx, "get_address_info(node, addr, timeout)");
    if (!JS_IsString(argv[0]) || !JS_IsString(argv[1]) || !JS_IsNumber(argv[2]))
        return JS_ThrowInternalError(ctx, "get_address_info: bad args");
    std::string node_addr = scope.toStdString(argv[0]);
    std::string addr = scope.toStdString(argv[1]);
    double to = 0;
    if (JS_ToFloat64(ctx, &to, argv[2])) return JS_ThrowInternalError(ctx, "timeout parse error");

    REF_getter<MsgData::GetAddressInfoREQ> rq = new MsgData::GetAddressInfoREQ();
    rq->addr.addr = base16::decode(addr);
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(node_addr, ServiceEnum::GrainReader,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}
#include "normalizeNick.h"

JSValue js_addr_from_name(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    JSScope<10,10> scope(ctx);
    if (argc != 1 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "addr_from_name(nick)");
    std::string nick = scope.toStdString(argv[0]);
    std::string norm = normalizeNick(nick);
    if (norm.empty())
        return JS_ThrowTypeError(ctx, "addr_from_name: bad nick (only ASCII a-zA-Z0-9_@.)");
    ADDRESS_id addr;
    addr.addr = blake2b_hash(norm).container;
    auto hex = base16::encode(addr.addr);
    return JS_NewStringLen(ctx, hex.data(), hex.size());
}
JSValue js_get_user_nonce(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
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

    REF_getter<MsgData::GetUserNonceREQ> rq = new MsgData::GetUserNonceREQ();
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

static std::string js_tx_wait_block_next_key()
{
    return std::string("next:");
}

JSValue js_tx_wait_block(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "tx_wait_block: !op");
    if (argc != 3)
        return JS_ThrowInternalError(ctx, "tx_wait_block(node, tx_hash, timeout)");
    if (!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "tx_wait_block: node must be string");
    if (!JS_IsString(argv[1]))
        return JS_ThrowInternalError(ctx, "tx_wait_block: tx_hash must be string");
    if (!JS_IsNumber(argv[2]))
        return JS_ThrowInternalError(ctx, "tx_wait_block: timeout must be number");

    std::string node_addr = scope.toStdString(argv[0]);
    std::string tx_hash_hex = scope.toStdString(argv[1]);
    double timeout = 0;
    if (JS_ToFloat64(ctx, &timeout, argv[2]))
        return JS_ThrowInternalError(ctx, "tx_wait_block: timeout parse error");

    if (op->tx_wait_promises.count(tx_hash_hex))
        return JS_ThrowTypeError(ctx, "tx_wait_block: already waiting for %s", tx_hash_hex.c_str());

    auto &pd = op->tx_wait_promises[tx_hash_hex];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    if (JS_IsException(promise)) {
        op->tx_wait_promises.erase(tx_hash_hex);
        return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    }
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    // logErr2("js_tx_wait_block: node=%s tx=%s timeout=%lf", node_addr.c_str(), tx_hash_hex.c_str(), timeout);
    op->broadcaster->sendEvent(node_addr, ServiceEnum::BlockStreamer,
                               new bcEvent::ClientTxSubscribeREQ(op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_TX_WAIT_BLOCK, toRef(tx_hash_hex), NULL, timeout, op->listener_));

    return g_prom.release();
}

JSValue js_get_block(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (!op)
        return JS_ThrowInternalError(ctx, "get_block: !op");
    if (argc != 2)
        return JS_ThrowInternalError(ctx, "get_block(node, timeout)");
    if (!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "get_block: node must be string");
    if (!JS_IsNumber(argv[1]))
        return JS_ThrowInternalError(ctx, "get_block: timeout must be number");

    std::string node_addr = scope.toStdString(argv[0]);
    double timeout = 0;
    if (JS_ToFloat64(ctx, &timeout, argv[1]))
        return JS_ThrowInternalError(ctx, "get_block: timeout parse error");

    std::string next_key = js_tx_wait_block_next_key();
    if (op->tx_wait_promises.count(next_key))
        return JS_ThrowTypeError(ctx, "get_block: another get_block is already waiting");

    auto &pd = op->tx_wait_promises[next_key];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    if (JS_IsException(promise)) {
        op->tx_wait_promises.erase(next_key);
        return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    }
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    pd.resolve = g_resolve;
    pd.reject = g_reject;

    op->broadcaster->sendEvent(node_addr, ServiceEnum::BlockStreamer,
                               new bcEvent::ClientTxSubscribeREQ(op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_TX_WAIT_BLOCK, toRef(next_key), NULL, timeout, op->listener_));

    return g_prom.release();
}

JSValue js_get_node_info(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "get_node_info: !op");
    if (argc != 3) return JS_ThrowInternalError(ctx, "get_node_info(node, name, timeout)");
    if (!JS_IsString(argv[0])) return JS_ThrowInternalError(ctx, "get_node_info: node must be string");
    if (!JS_IsString(argv[1])) return JS_ThrowInternalError(ctx, "get_node_info: name must be string");
    if (!JS_IsNumber(argv[2])) return JS_ThrowInternalError(ctx, "get_node_info: timeout must be number");
    std::string node_addr = scope.toStdString(argv[0]);
    std::string name = scope.toStdString(argv[1]);
    double to = 0;
    if (JS_ToFloat64(ctx, &to, argv[2])) return JS_ThrowInternalError(ctx, "get_node_info: timeout parse error");

    REF_getter<MsgData::GetNodeInfoREQ> rq = new MsgData::GetNodeInfoREQ();
    rq->node_name.container = name;
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(node_addr, ServiceEnum::GrainReader,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}

JSValue js_get_values(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "get_values: !op");
    if (argc != 2) return JS_ThrowInternalError(ctx, "get_values(node, timeout)");
    if (!JS_IsString(argv[0])) return JS_ThrowInternalError(ctx, "get_values: node must be string");
    if (!JS_IsNumber(argv[1])) return JS_ThrowInternalError(ctx, "get_values: timeout must be number");
    std::string node_addr = scope.toStdString(argv[0]);
    double to = 0;
    if (JS_ToFloat64(ctx, &to, argv[1])) return JS_ThrowInternalError(ctx, "get_values: timeout parse error");

    REF_getter<MsgData::GetValuesREQ> rq = new MsgData::GetValuesREQ();
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(node_addr, ServiceEnum::GrainReader,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}

JSValue js_get_contract_data(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "get_contract_data: !op");
    if (argc != 4) return JS_ThrowInternalError(ctx, "get_contract_data(node, contract, key, timeout)");
    for (int i = 0; i < 3; i++) if (!JS_IsString(argv[i])) return JS_ThrowInternalError(ctx, "get_contract_data: arg %d must be string", i);
    if (!JS_IsNumber(argv[3])) return JS_ThrowInternalError(ctx, "get_contract_data: timeout must be number");
    std::string node_addr = scope.toStdString(argv[0]);
    std::string contract = scope.toStdString(argv[1]);
    std::string key = scope.toStdString(argv[2]);
    double to = 0;
    if (JS_ToFloat64(ctx, &to, argv[3])) return JS_ThrowInternalError(ctx, "get_contract_data: timeout parse error");

    REF_getter<MsgData::GetContractDataREQ> rq = new MsgData::GetContractDataREQ();
    rq->contract.container = contract;
    rq->key.container = key;
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(node_addr, ServiceEnum::GrainReader,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}

JSValue js_call_view(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10,10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque*)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "call_view: !op");
    if (argc != 6) return JS_ThrowInternalError(ctx, "call_view(node, contract, view, params, timeout_sec, timeout)");
    if (!JS_IsString(argv[0]) || !JS_IsString(argv[1]) || !JS_IsString(argv[2]) ||
        !JS_IsObject(argv[3]) || !JS_IsNumber(argv[4]) || !JS_IsNumber(argv[5]))
        return JS_ThrowInternalError(ctx, "call_view: bad args");

    std::string node_addr = scope.toStdString(argv[0]);
    std::string contract  = scope.toStdString(argv[1]);
    std::string view      = scope.toStdString(argv[2]);
    std::string params_json;
    qjs::convert_js_value_to_json(ctx, argv[3], params_json);
    int64_t timeout_sec = 5;
    if (JS_ToInt64(ctx, &timeout_sec, argv[4])) return JS_ThrowInternalError(ctx, "call_view: timeout_sec parse error");
    double to = 0;
    if (JS_ToFloat64(ctx, &to, argv[5])) return JS_ThrowInternalError(ctx, "call_view: timeout parse error");

    REF_getter<MsgData::CallViewREQ> rq = new MsgData::CallViewREQ();
    rq->contract.container = contract;
    rq->view = view;
    rq->params_json = params_json;
    rq->timeout_sec = timeout_sec;
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(node_addr, ServiceEnum::GrainReader,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}


JSValue js_get_state(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "get_state: !op");
    if (argc != 2) return JS_ThrowInternalError(ctx, "get_state(node, timeout)");
    if (!JS_IsString(argv[0])) return JS_ThrowInternalError(ctx, "get_state: node must be string");
    if (!JS_IsNumber(argv[1])) return JS_ThrowInternalError(ctx, "get_state: timeout must be number");
    std::string node_addr = scope.toStdString(argv[0]);
    double to = 0;
    if (JS_ToFloat64(ctx, &to, argv[1])) return JS_ThrowInternalError(ctx, "get_state: timeout parse error");

    REF_getter<MsgData::GetStateREQ> rq = new MsgData::GetStateREQ();
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(node_addr, ServiceEnum::GrainReader,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}

// в js_register_add_instance:

JSValue js_tx_unsubscribe(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSScope<10, 10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "!op");
    if (argc < 1 || argc > 2) return JS_ThrowInternalError(ctx, "tx_unsubscribe(node[, tx_hash])");
    if (!JS_IsString(argv[0])) return JS_ThrowInternalError(ctx, "node must be string");
    std::string node_addr = scope.toStdString(argv[0]);
    THASH_id h;
    if (argc == 2 && JS_IsString(argv[1])) {
        auto hex = scope.toStdString(argv[1]);
        h.container = base16::decode(hex);
    }
    op->broadcaster->sendEvent(node_addr, ServiceEnum::BlockStreamer,
                               new bcEvent::ClientTxUnsubscribeREQ(h, op->listener_->serviceId));
    if (h.container.empty()) op->tx_subscription_cb.reset();
    return JS_UNDEFINED;
}
JSValue js_get_granule(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10,10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque*)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "get_granule: !op");
    if (argc != 4) return JS_ThrowInternalError(ctx, "get_granule(node, addr_hex, hash_hex, timeout)");
    if (!JS_IsString(argv[0]) || !JS_IsString(argv[1]) || !JS_IsString(argv[2]) || !JS_IsNumber(argv[3]))
        return JS_ThrowInternalError(ctx, "get_granule: bad args");

    std::string node_addr = scope.toStdString(argv[0]);
    std::string addr_hex  = scope.toStdString(argv[1]);
    std::string hash_hex  = scope.toStdString(argv[2]);
    double to = 0;
    if (JS_ToFloat64(ctx, &to, argv[3])) return JS_ThrowInternalError(ctx, "get_granule: timeout parse error");

    REF_getter<MsgData::GetGranuleREQ> rq = new MsgData::GetGranuleREQ();
    rq->addr = base16::decode(addr_hex);
    rq->hash = hash_hex.empty() ? "" : base16::decode(hash_hex);
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(node_addr, ServiceEnum::GrainReader,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}
JSValue js_get_granule_lite(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10,10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque*)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "get_granule_lite: !op");
    if (argc != 3) return JS_ThrowInternalError(ctx, "get_granule_lite(addr_hex, hash_hex, timeout)");
    if (!JS_IsString(argv[0]) || !JS_IsString(argv[1]) || !JS_IsNumber(argv[2]))
        return JS_ThrowInternalError(ctx, "get_granule_lite: bad args");

    // std::string node_addr = scope.toStdString(argv[0]);
    std::string addr_hex  = scope.toStdString(argv[0]);
    std::string hash_hex  = scope.toStdString(argv[1]);
    double to = 0;
    if (JS_ToFloat64(ctx, &to, argv[2])) return JS_ThrowInternalError(ctx, "get_granule: timeout parse error");

    REF_getter<MsgData::GetGranuleREQ> rq = new MsgData::GetGranuleREQ();
    rq->addr = base16::decode(addr_hex);
    rq->hash = hash_hex.empty() ? "" : base16::decode(hash_hex);
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(ServiceEnum::GrainReaderLite,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, to, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}
JSValue js_lite_set_uplink(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope<10,10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque*)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "lite_set_uplink: !op");
    if (argc != 1 || !JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "lite_set_uplink(addr)");
    std::string addr = scope.toStdString(argv[0]);
    if (addr.empty())
        return JS_ThrowInternalError(ctx, "lite_set_uplink: addr empty");
    op->broadcaster->sendEvent(ServiceEnum::GrainReaderLite,
                               new bcEvent::LiteSetUplinkREQ(addr, op->listener_->serviceId));
    return JS_UNDEFINED;
}
JSValue js_scan_expired(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    JSScope<10,10> scope(ctx);
    mtjs_opaque *op = (mtjs_opaque*)JS_GetContextOpaque(ctx);
    if (!op) return JS_ThrowInternalError(ctx, "scan_expired: !op");
    if (argc != 3) return JS_ThrowInternalError(ctx, "scan_expired(node, from_hex, max_sec)");
    if (!JS_IsString(argv[0]) || !JS_IsString(argv[1]) || !JS_IsNumber(argv[2]))
        return JS_ThrowInternalError(ctx, "scan_expired: bad args");
    std::string node_addr = scope.toStdString(argv[0]);
    std::string from_hex  = scope.toStdString(argv[1]);
    int64_t max_sec = 0;
    if (JS_ToInt64(ctx, &max_sec, argv[2])) return JS_ThrowInternalError(ctx, "scan_expired: max_sec parse error");

    REF_getter<MsgData::ScanREQ> rq = new MsgData::ScanREQ();
    rq->from.container = from_hex.empty() ? "" : base16::decode(from_hex);
    rq->max_sec = max_sec;
    rq->now = 0;
    auto hash = rq->getHash();
    op->broadcaster->sendEvent(node_addr, ServiceEnum::Scan,
                               new bcEvent::ClientMsg(rq->getBuffer(), op->listener_->serviceId));
    op->broadcaster->sendEvent(ServiceEnum::Timer,
                               new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(hash.container), NULL, 30.0, op->listener_));

    auto &pd = op->node_req_promises[hash.container];
    pd.ctx = ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    JSValueGuard g_resolve(ctx, prom[0]);
    JSValueGuard g_reject(ctx, prom[1]);
    JSValueGuard g_prom(ctx, promise);
    if (JS_IsException(promise)) return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
    pd.resolve = g_resolve;
    pd.reject = g_reject;
    return g_prom.release();
}
// jsi/api/contract_api.cpp

// createInstance.cpp
#include <sodium.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>

JSValue js_generate_keypair(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    int64_t type = 0;
    if (argc >= 1 && !JS_IsUndefined(argv[0])) {
        if (JS_ToInt64(ctx, &type, argv[0]) != 0) {
            return JS_ThrowTypeError(ctx, "generate_keypair: type must be int");
        }
    }
    if (type < 0 || type > 3) {
        return JS_ThrowTypeError(ctx,
            "generate_keypair: unknown type %lld (0=ed25519, 1=secp256k1, 2=p256, 3=bls12-381)",
            (long long)type);
    }

    std::string sk_hex, pk_hex;

    if (type == 0) {
        // ed25519 (libsodium)
        unsigned char pk[crypto_sign_PUBLICKEYBYTES];   // 32
        unsigned char sk[crypto_sign_SECRETKEYBYTES];   // 64
        if (crypto_sign_keypair(pk, sk) != 0) {
            return JS_ThrowInternalError(ctx, "generate_keypair: crypto_sign_keypair failed");
        }
        sk_hex = base16::encode(std::string((char*)sk, sizeof(sk)));
        pk_hex = base16::encode(std::string((char*)pk, sizeof(pk)));
    } else if (type == 1 || type == 2) {
        // secp256k1 / p256 (OpenSSL)
        int nid = (type == 1) ? NID_secp256k1 : NID_X9_62_prime256v1;
        EC_KEY* ec = EC_KEY_new_by_curve_name(nid);
        if (!ec) {
            return JS_ThrowInternalError(ctx, "generate_keypair: EC_KEY_new_by_curve_name failed");
        }
        if (!EC_KEY_generate_key(ec)) {
            EC_KEY_free(ec);
            return JS_ThrowInternalError(ctx, "generate_keypair: EC_KEY_generate_key failed");
        }

        // sk: 32 байта (BN, big-endian, pad до 32)
        const BIGNUM* priv = EC_KEY_get0_private_key(ec);
        std::string sk_bin(32, '\0');
        BN_bn2binpad(priv, (unsigned char*)&sk_bin[0], 32);
        sk_hex = base16::encode(sk_bin);

        // pk: compressed (33 байта)
        const EC_GROUP* group = EC_KEY_get0_group(ec);
        const EC_POINT* pub = EC_KEY_get0_public_key(ec);
        size_t pklen = EC_POINT_point2oct(group, pub, POINT_CONVERSION_COMPRESSED, nullptr, 0, nullptr);
        std::string pk_bin(pklen, '\0');
        EC_POINT_point2oct(group, pub, POINT_CONVERSION_COMPRESSED, (unsigned char*)&pk_bin[0], pklen, nullptr);
        pk_hex = base16::encode(pk_bin);

        EC_KEY_free(ec);
    } else if (type == 3) {
        // BLS12-381 (blst) — sk 32 байта, pk 48 байт compressed G1
        unsigned char ikm[32];
        randombytes_buf(ikm, sizeof(ikm));

        blst_scalar sk;
        blst_keygen(&sk, ikm, sizeof(ikm), nullptr, 0);

        blst_p1 pk;
        blst_sk_to_pk_in_g1(&pk, &sk);

        sk_hex = base16::encode(std::string((char*)sk.b, 32));

        std::string pk_bin(48, '\0');
        blst_p1_compress((unsigned char*)&pk_bin[0], &pk);
        pk_hex = base16::encode(pk_bin);
    } else {
        // unreachable (защита)
        return JS_ThrowInternalError(ctx, "generate_keypair: unreachable type=%lld", (long long)type);
    }

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "type", JS_NewInt64(ctx, type));
    JS_SetPropertyStr(ctx, obj, "sk",   JS_NewString(ctx, sk_hex.c_str()));
    JS_SetPropertyStr(ctx, obj, "pk",   JS_NewString(ctx, pk_hex.c_str()));
    return obj;
}
// register:
// в register_api:
void js_register_add_instance(JSContext *ctx, JSValue &mtjs_obj)
{
    MUTEX_INSPECTOR;
    // logErr2("js_register_add_instance");
    JS_SetPropertyStr(ctx, mtjs_obj, "addInstance",       JS_NewCFunction(ctx, js_add_instance,       "addInstance", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "killInstance",     JS_NewCFunction(ctx, js_kill_instance,     "killInstance", 1));
    JS_SetPropertyStr(ctx, mtjs_obj, "tx_submit",         JS_NewCFunction(ctx, js_tx_submit,          "tx_submit", 6));
    JS_SetPropertyStr(ctx, mtjs_obj, "tx_subscribe",      JS_NewCFunction(ctx, js_tx_subscribe,       "tx_subscribe", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_user_nonce",    JS_NewCFunction(ctx, js_get_user_nonce,     "get_user_nonce", 3));
    JS_SetPropertyStr(ctx, mtjs_obj, "tx_wait_block",     JS_NewCFunction(ctx, js_tx_wait_block,      "tx_wait_block", 3));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_block",         JS_NewCFunction(ctx, js_get_block,          "get_block", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_node_info",     JS_NewCFunction(ctx, js_get_node_info,      "get_node_info", 3));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_values",        JS_NewCFunction(ctx, js_get_values,         "get_values", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "tx_unsubscribe", JS_NewCFunction(ctx, js_tx_unsubscribe, "tx_unsubscribe", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_contract_data", JS_NewCFunction(ctx, js_get_contract_data,  "get_contract_data", 4));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_contract", JS_NewCFunction(ctx, js_get_contract, "get_contract", 3));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_state", JS_NewCFunction(ctx, js_get_state, "get_state", 2));
// в js_register_add_instance:
    JS_SetPropertyStr(ctx, mtjs_obj, "get_address_info", JS_NewCFunction(ctx, js_get_address_info, "get_address_info", 3));
    JS_SetPropertyStr(ctx, mtjs_obj, "addr_from_name", JS_NewCFunction(ctx, js_addr_from_name, "addr_from_name", 1));
    JS_SetPropertyStr(ctx, mtjs_obj, "call_view", JS_NewCFunction(ctx, js_call_view, "call_view", 6));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_granule", JS_NewCFunction(ctx, js_get_granule, "get_granule", 4));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_granule_lite", JS_NewCFunction(ctx, js_get_granule_lite, "get_granule_lite", 3));
    JS_SetPropertyStr(ctx, mtjs_obj, "lite_set_uplink", JS_NewCFunction(ctx, js_lite_set_uplink, "lite_set_uplink", 1));
    JS_SetPropertyStr(ctx, mtjs_obj, "blake2b_hex", JS_NewCFunction(ctx, js_blake2b_hex, "blake2b_hex", 1));
    JS_SetPropertyStr(ctx, mtjs_obj, "sign_ed",     JS_NewCFunction(ctx, js_sign_ed,     "sign_ed",     2));
    JS_SetPropertyStr(ctx, mtjs_obj, "scan_expired", JS_NewCFunction(ctx, js_scan_expired, "scan_expired", 3));
    JS_SetPropertyStr(ctx, mtjs_obj, "generate_keypair", JS_NewCFunction(ctx, js_generate_keypair, "generate_keypair", 1));

}
