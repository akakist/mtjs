#include <quickjs.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "common/async_task.h"
#include "common/jsscope.h"
#include "common/mtjs_opaque.h"
#include "main/configObj.h"
#include "bcEvent.h"

#include "quickjs.h"

#include "quickjs.h"
#include <string.h>
#include <stdio.h>

#include "quickjs.h"
#include <string>
#include <vector>
#include <cstdio>
#include <nlohmann/json.hpp>
#include "msg.h"
#include "msg_tx.h"
#include "Events/System/timerEvent.h"
#include "timers.h"
// #include ""

static std::string js_obj_to_kv(JSContext *ctx,
                                JSValueConst obj) {

    // if (argc < 1 || !JS_IsObject(argv[0])) {
    //     return JS_ThrowTypeError(ctx, "Expected an object");
    // }

    JSScope <100,100> scope(ctx);
    JSPropertyEnum *props;
    uint32_t len;
    if (JS_GetOwnPropertyNames(ctx, &props, &len, obj,
                               JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) {
        throw CommonError("JS_GetOwnPropertyNames(ctx, &props, &len, obj,");
    }

    std::string result;

    for (uint32_t i = 0; i < len; i++) {
        const char *key = JS_AtomToCString(ctx, props[i].atom);
        JSValue val = JS_GetProperty(ctx, obj, props[i].atom);
        const char *val_str = JS_ToCString(ctx, val);

        if (key && val_str) {
            result += key;
            result += "=";
            result += val_str;
            if (i + 1 < len) {
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

    try {
        JSScope <10,10> scope(ctx);
        mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
        if(!op)
            return JS_ThrowInternalError(ctx, "!op");
        op->rpcBlockExit=true;

        if(argc !=2)
            return JS_ThrowInternalError(ctx, "number of argument must be 2");

        std::string iname=(std::string)scope.toStdStringView(argv[0]);

        std::string cnf=(std::string)scope.toStdStringView(argv[1]);

        // if(!JS_IsObject(argv[0]))
        //     return JS_ThrowTypeError(ctx, "if(!JS_IsObject(argv[0]))");

        IInstance *instance1=iUtils->createNewInstance(iname);
        ConfigObj *cnf1=new ConfigObj(iname,cnf);
        instance1->setConfig(cnf1);
        instance1->initServices();

        return JS_UNDEFINED;
    } catch(std::exception &e)
    {
        logErr2("exception %s",e.what());
        return JS_ThrowInternalError(ctx, "exception %s",e.what());
    }
    return JS_ThrowInternalError(ctx, "return values wrong");
}
JSValue js_tx_subscribe(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSScope <10,10> scope(ctx);
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op)
        return JS_ThrowInternalError(ctx, "!op");
    if(argc !=2)
        return JS_ThrowInternalError(ctx, "number of argument must be 2");
    if(!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "node addr not specified");
    if(!JS_IsFunction(ctx,argv[1]))
        return JS_ThrowInternalError(ctx, "callback not specified");
    std::string node_addr=(std::string)scope.toStdStringView(argv[0]);
    JSValue cb=argv[1];
    op->tx_subscription_cb.emplace(JHolder(ctx,cb));

    op->broadcaster->sendEvent(node_addr,ServiceEnum::BlockStreamer, new bcEvent::ClientTxSubscribeREQ(op->listener_->serviceId));

    // op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT,toRef(hash.container),NULL,to,op->listener_));
    return JS_UNDEFINED;
}
JSValue js_mint(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{

    JSScope <10,10> scope(ctx);
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(argc !=3)
        return JS_ThrowInternalError(ctx, "number of argument must be 2");


    if(!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "node addr not specified");
    if(!JS_IsString(argv[1]))
        return JS_ThrowInternalError(ctx, "sk not specified");
    if(!JS_IsObject(argv[2]))
        return JS_ThrowInternalError(ctx, "msg Object not specified");

    std::string node_addr=scope.toStdString(argv[0]);
    std::string sk=base62::decode(scope.toStdString(argv[1]));


    // JSValue sk = JS_GetPropertyStr(ctx, argv[2], "sk");
    // scope.addValue(sk);
    // if(!JS_IsString(sk))
    //     return JS_ThrowInternalError(ctx, "msg wrong param sk");
    JSValue amount = JS_GetPropertyStr(ctx, argv[2], "amount");
    scope.addValue(amount);
    if(!JS_IsString(amount))
        return JS_ThrowInternalError(ctx, "msg wrong param amount");
    JSValue nonce = JS_GetPropertyStr(ctx, argv[2], "nonce");
    scope.addValue(nonce);

    if(!JS_IsString(nonce))
        return JS_ThrowInternalError(ctx, "msg wrong param nonce");
    JSValue timeout = JS_GetPropertyStr(ctx, argv[2], "timeout");
    scope.addValue(timeout);
    if(!JS_IsNumber(timeout))
        return JS_ThrowInternalError(ctx, "msg wrong param timeout");
    //  if(!JS_IsString(sk) || ! JS_IsString(amount) || !JS_IsString(nonce) || !JS_IsNumber(timeout))
    //  {
    //     return JS_ThrowInternalError(ctx, "msg wrong params");
    //  }

    // auto _sk=scope.toStdStringView(sk);
    auto _amount=scope.toStdStringView(amount);
    auto _nonce=scope.toStdStringView(nonce);
    double to;
    if(JS_ToFloat64(ctx,&to,timeout))
        return JS_ThrowInternalError(ctx, "timeout parse error");



    tx::mint m;
    m.amount.from_string(std::string(_amount));

    msg::user_message_req um;
    // if(um.payload.size()==1)
    // crypto_sign_ed25519_sk_to_seed(seed, sk.data());
    if(sk.size()!=crypto_sign_SECRETKEYBYTES)
        return JS_ThrowRangeError(ctx, "if(sk.size()!=crypto_sign_SECRETKEYBYTES)");
    unsigned char extracted_public[crypto_sign_PUBLICKEYBYTES];
    crypto_sign_ed25519_sk_to_pk(extracted_public, (uint8_t*)sk.data());
    auto pk=std::string((char*)extracted_public,crypto_sign_PUBLICKEYBYTES);
    um.address_pk_ed=pk;
    um.payload.push_back(m.getBuffer());
    um.nonce.from_string((std::string)_nonce);
    // auto skk=iUtils->hex2bin(_sk);
    // if(skk.size()!=crypto_sign_SECRETKEYBYTES)
    //     return JS_ThrowInternalError(ctx, "sk size invalid");




    // um.pk.resize(crypto_sign_PUBLICKEYBYTES);
    // crypto_sign_ed25519_sk_to_pk((uint8_t*)um.pk.data(), (unsigned char*)skk.data());
    um.sign(sk);

    auto buf=um.getBuffer();
    auto hash=blake2b_hash(buf);
    op->broadcaster->sendEvent(node_addr,ServiceEnum::TxValidator, new bcEvent::ClientMsg(buf, op->listener_->serviceId));

    // logErr2("setalarm %lf",to);
    op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT,toRef(hash.container),NULL,to,op->listener_));


    auto &pd=op->node_req_promises[hash];
    pd.ctx=ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }
    pd.resolve=JS_DupValue(ctx, prom[0]);
    pd.reject=JS_DupValue(ctx, prom[1]);


    return promise;

}

////////////////
JSValue js_tx_submit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{

    JSScope <10,10> scope(ctx);
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(argc !=5)
        return JS_ThrowInternalError(ctx, "number of argument must be 2");


    if(!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "node addr not specified");
    if(!JS_IsString(argv[1]))
        return JS_ThrowInternalError(ctx, "nonce not specified");
    if(!JS_IsString(argv[2]))
        return JS_ThrowInternalError(ctx, "sk not specified");
    if(!JS_IsNumber(argv[3]))
        return JS_ThrowInternalError(ctx, "timeout not specified");
    if(!JS_IsArray(ctx, argv[4]))
        return JS_ThrowInternalError(ctx, "msg array not specified");

    auto node_addr=scope.toStdString(argv[0]);
    logErr2("node_addr %s",node_addr.c_str());
    auto nonce=scope.toStdString(argv[2]);
    logErr2("nonce %s",nonce.c_str());
    std::string sk=base62::decode(scope.toStdString(argv[3]));
    int64_t timeout;
    if(JS_ToInt64(ctx,& timeout, argv[4]))
    {
        return JS_ThrowInternalError(ctx, "error parsing timeout");
    }
    if(sk.size()!=crypto_sign_SECRETKEYBYTES)
        return JS_ThrowRangeError(ctx, "if(sk.size()!=crypto_sign_SECRETKEYBYTES)");
    unsigned char extracted_public[crypto_sign_PUBLICKEYBYTES];
    crypto_sign_ed25519_sk_to_pk(extracted_public, (uint8_t*)sk.data());
    auto pk=std::string((char*)extracted_public,crypto_sign_PUBLICKEYBYTES);

    msg::user_message_req um;
    um.address_pk_ed=pk;

    // std::vector<std::string> trs;
    JSValue length_val = JS_GetPropertyStr(ctx, argv[5], "length");
    scope.addValue(length_val);
    uint32_t len;
    JS_ToUint32(ctx, &len, length_val);
    for (uint32_t i = 0; i < len; ++i) {
        // if (i > 0) result.append(", ");
        JSValue item = JS_GetPropertyUint32(ctx, argv[4], i);
        scope.addValue(item);

        JSValue _type=JS_GetPropertyStr(ctx,item,"type");
        scope.addValue(_type);

        if(!JS_IsString(_type))
            return JS_ThrowSyntaxError(ctx, "type not specified");
        auto type=scope.toStdString(_type);
        if(type=="mint")
        {
            JSValue _amount=JS_GetPropertyStr(ctx,item,"amount");
            scope.addValue(_amount);
            if(!JS_IsString(_amount))
                return JS_ThrowSyntaxError(ctx, "amount not specified");
            auto amount=scope.toStdString(_amount);
            tx::mint m;
            m.amount.from_string(amount);
            um.payload.push_back(m.getBuffer());
        }
        if(type=="register_node")
        {
            JSValue _name=JS_GetPropertyStr(ctx,item,"name");
            scope.addValue(_name);
            if(!JS_IsString(_name))
                return JS_ThrowSyntaxError(ctx, "name not specified");
            auto name=scope.toStdString(_name);

            JSValue _ip=JS_GetPropertyStr(ctx,item,"ip");
            scope.addValue(_ip);
            if(!JS_IsString(_ip))
                return JS_ThrowSyntaxError(ctx, "ip not specified");
            auto ip=scope.toStdString(_ip);

            JSValue _pk_ed=JS_GetPropertyStr(ctx,item,"pk_ed");
            scope.addValue(_pk_ed);
            if(!JS_IsString(_pk_ed))
                return JS_ThrowSyntaxError(ctx, "ip not specified");
            auto pk_ed=scope.toStdString(_pk_ed);

            JSValue _pk_bls=JS_GetPropertyStr(ctx,item,"pk_bls");
            scope.addValue(_pk_bls);
            if(!JS_IsString(_pk_bls))
                return JS_ThrowSyntaxError(ctx, "pk_bls not specified");
            auto pk_bls=scope.toStdString(_pk_bls);

            tx::registerNode m;
            m.name.container=name;
            m.ip=ip;
            m.pk_bls.deserializeBase62Str(pk_bls);
            m.pk_ed=base62::decode(pk_ed);

            um.payload.push_back(m.getBuffer());
        }
        if(type=="stake")
        {
            JSValue _node=JS_GetPropertyStr(ctx,item,"node");
            scope.addValue(_node);
            if(!JS_IsString(_node))
                return JS_ThrowSyntaxError(ctx, "node not specified");
            auto node=scope.toStdString(_node);

            JSValue _amount=JS_GetPropertyStr(ctx,item,"amount");
            scope.addValue(_amount);
            if(!JS_IsString(_amount))
                return JS_ThrowSyntaxError(ctx, "amount not specified");
            auto amount=scope.toStdString(_amount);

            tx::stake m;
            m.node.container=node;
            m.amount.from_string(amount);

            um.payload.push_back(m.getBuffer());
        }
        if(type=="unstake")
        {
            JSValue _node=JS_GetPropertyStr(ctx,item,"node");
            scope.addValue(_node);
            if(!JS_IsString(_node))
                return JS_ThrowSyntaxError(ctx, "node not specified");
            auto node=scope.toStdString(_node);

            JSValue _amount=JS_GetPropertyStr(ctx,item,"amount");
            scope.addValue(_amount);
            if(!JS_IsString(_amount))
                return JS_ThrowSyntaxError(ctx, "amount not specified");
            auto amount=scope.toStdString(_amount);

            tx::unstake m;
            m.node.container=node;
            m.amount.from_string(amount);

            um.payload.push_back(m.getBuffer());
        }
        if(type=="transfer")
        {
            JSValue _to=JS_GetPropertyStr(ctx,item,"to");
            scope.addValue(_to);
            if(!JS_IsString(_to))
                return JS_ThrowSyntaxError(ctx, "to not specified");
            auto to=base62::decode(scope.toStdString(_to));

            JSValue _amount=JS_GetPropertyStr(ctx,item,"amount");
            scope.addValue(_amount);
            if(!JS_IsString(_amount))
                return JS_ThrowSyntaxError(ctx, "amount not specified");
            auto amount=scope.toStdString(_amount);

            tx::transfer m;
            m.to_address=to;
            m.amount.from_string(amount);
            um.payload.push_back(m.getBuffer());
        }
        if(type=="register_contract")
        {
            JSValue _name=JS_GetPropertyStr(ctx,item,"name");
            scope.addValue(_name);
            if(!JS_IsString(_name))
                return JS_ThrowSyntaxError(ctx, "name not specified");
            auto name=scope.toStdString(_name);

            JSValue _src=JS_GetPropertyStr(ctx,item,"src");
            scope.addValue(_src);
            if(!JS_IsString(_src))
                return JS_ThrowSyntaxError(ctx, "amount not specified");
            auto src=scope.toStdString(_src);

            tx::createContract m;
            m.name=name;
            m.src=src;
            um.payload.push_back(m.getBuffer());
        }
        // if(type=="register_user")
        // {
        //     JSValue _name=JS_GetPropertyStr(ctx,item,"nick");
        //     scope.addValue(_name);
        //     if(!JS_IsString(_name))
        //         return JS_ThrowSyntaxError(ctx, "name not specified");
        //     tx::registerUser m;
        //     m.nickName=scope.toStdString(_name);
        //     um.payload.push_back(m.getBuffer());
        // }


    }
    um.nonce.from_string(nonce);
    um.sign(sk);
    std::string buf=um.getBuffer();
    auto hash=blake2b_hash(buf);
    op->broadcaster->sendEvent(node_addr,ServiceEnum::TxValidator, new bcEvent::ClientMsg(buf, op->listener_->serviceId));

    op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT,toRef(hash.container),NULL,timeout,op->listener_));


    auto &pd=op->node_req_promises[hash];
    pd.ctx=ctx;
    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }
    pd.resolve=JS_DupValue(ctx, prom[0]);
    pd.reject=JS_DupValue(ctx, prom[1]);


    return promise;

}
#include <openssl/rand.h>
/////////////////
JSValue js_get_user_info(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JSScope <10,10> scope(ctx);
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(argc !=3)
        return JS_ThrowInternalError(ctx, "number of argument must be 3");

    if(!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx, "node addr not specified");
    if(!JS_IsString(argv[1]))
        return JS_ThrowInternalError(ctx, "nick not specified");
    if(!JS_IsNumber(argv[2]))
        return JS_ThrowInternalError(ctx, "timeout not specified");

    std::string node_addr=(std::string)scope.toStdString(argv[0]);
    std::string address=base62::decode(scope.toStdString(argv[1]));
    // std::string nick=(std::string)scope.toStdString(argv[1]);
    double to;
    if(JS_ToFloat64(ctx,&to,argv[2]))
        return JS_ThrowInternalError(ctx, "timeout parse error");

    //  JSValue sk = JS_GetPropertyStr(ctx, argv[1], "sk");
    //  scope.addValue(sk);
    //  JSValue nick = JS_GetPropertyStr(ctx, argv[1], "nick");
    //  scope.addValue(nick);
    //  JSValue timeout = JS_GetPropertyStr(ctx, argv[1], "timeout");
    //  scope.addValue(timeout);
    //  if(!JS_IsString(sk))
    //  {
    //     return JS_ThrowInternalError(ctx, "sk not found");
    //  }
    //  if(!JS_IsString(nick))
    //  {
    //     return JS_ThrowInternalError(ctx, "nick not found");
    //  }
    //  if( !JS_IsNumber(timeout))
    //  {
    //     return JS_ThrowInternalError(ctx, "timeout not found");
    //  }
    //  auto _sk=scope.toStdStringView(sk);

    msg::user_request ur;
    ur.rnd.resize(10);
    RAND_bytes((unsigned char*)ur.rnd.data(),10);
    msg::get_user_status_req m;
    m.address_pk_ed=address;

    ur.payload=m.getBuffer();

    auto hash=blake2b_hash(ur.getBuffer());
    op->broadcaster->sendEvent(scope.toStdString(argv[0]),ServiceEnum::GrainReader, new bcEvent::ClientMsg(ur.getBuffer(), op->listener_->serviceId));

    op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(Timers::TIMER_ClientMsg_TIMEDOUT,toRef(hash.container),NULL,to,op->listener_));

    auto &pd=op->node_req_promises[hash];
    pd.ctx=ctx;

    JSValue prom[2];
    JSValue promise = JS_NewPromiseCapability(ctx, prom);
    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }
    pd.resolve=JS_DupValue(ctx, prom[0]);
    pd.reject=JS_DupValue(ctx, prom[1]);
    return promise;

}


void js_register_add_instance( JSContext *ctx,JSValue & mtjs_obj)
{
    MUTEX_INSPECTOR;
    logErr2("js_register_add_instance");
    JS_SetPropertyStr(ctx, mtjs_obj, "addInstance", JS_NewCFunction(ctx, js_add_instance, "addInstance", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "mint", JS_NewCFunction(ctx, js_mint, "mint", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "tx_submit", JS_NewCFunction(ctx, js_tx_submit, "tx_submit", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "tx_subscribe", JS_NewCFunction(ctx, js_tx_subscribe, "tx_subscribe", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "get_user_info", JS_NewCFunction(ctx, js_get_user_info, "get_user_info", 2));
}
