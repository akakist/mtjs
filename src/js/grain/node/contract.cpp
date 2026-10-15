#include "nodeService.h"
#include "yyjson_to_quickjs.h"

std::optional<std::string> Node::Service::execute_contract(const CONTRACT_id& ct, const std::string & method, yyjson_val* params)
{
    auto it=contracts.find(ct);
    if(it==contracts.end())
    {
        auto err=load_contract(ct);
        if(err)
            return err;
        it=contracts.find(ct);
        if(it==contracts.end())
            throw CommonError("if(it--contracts.end())");
    }
    JSScope<10, 10> scope(it->second->ctx);
    YYJsonToQuickJS converter(it->second->ctx);
    JSValue jspars=converter.convert(params);
    scope.addValue(jspars);
    auto mi=it->second->methods.find(method);
    if(mi==it->second->methods.end())
        return "method not found";
    auto ret=JS_Call(it->second->ctx,mi->second.get(),JS_UNDEFINED,1, &jspars);
    scope.addValue(ret);
    std::string err;

    if (qjs::CheckAndGetException(it->second->ctx, ret, "loadModule",err))
        return err;

    return std::nullopt;
}
#include "jsscope.h"
#include "js_tools.h"
std::optional<std::string> Node::Service::load_contract(const CONTRACT_id& contract)
{
    auto c=db_state->getContract(contract);
    REF_getter<contract_rt> ct=new contract_rt();
    ct->ctx=JS_NewContext(contract_runtime);
    {
        M_LOCK(c->parent->mx);
        ct->src=c->src;
        ct->owner=c->owner;
    }

    JSScope<10, 10> scope(ct->ctx);
    JSValue module1 = JS_Eval(ct->ctx, ct->src.data(), ct->src.size(), "<module>", JS_EVAL_FLAG_STRICT | JS_EVAL_TYPE_MODULE | JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
    scope.addValue(module1);

    std::string err;
    if (qjs::CheckAndGetException(ct->ctx, module1, "loadModule",err))
        return err;

    JSValue ret = JS_EvalFunction(ct->ctx, module1);
    scope.addValue(ret);
    if (qjs::CheckAndGetException(ct->ctx, ret, "loadModule",err))
        return err;

    contracts.insert_or_assign(contract,ct);

    return std::nullopt;
}
