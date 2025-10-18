/*
 * js_mysql.cpp - QuickJS native module for MySQL
 * Requires: MySQL C API (libmysqlclient)
 */

extern "C" {
#include "quickjs.h"
}

#include <mariadb/mysql.h>
#include "jsscope.h"

#include "quickjs.h"
#include <mariadb/mysql.h>
// #include "json/json.h"
// Json::Value jsvalue_to_json(JSContext* ctx, JSValueConst val);

struct  MySQLConnection {
    MYSQL *conn=nullptr;
};

static JSClassID js_mysql_class_id;

static void js_mysql_finalizer(JSRuntime *rt, JSValue val) {
    MySQLConnection *s = (MySQLConnection *)JS_GetOpaque(val, js_mysql_class_id);
    if (s) {
        if (s->conn) mysql_close(s->conn);
        delete s;
    }
}

static JSValue js_mysql_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    JSScope <10,10> scope(ctx);
    JSValue obj = JS_UNDEFINED;

    if(!JS_IsObject(argv[0])) {
        return JS_ThrowTypeError(ctx, "Expected (Object )");
    }
    auto hostVal=JS_GetPropertyStr(ctx, argv[0], "host");
    scope.addValue(hostVal);
    auto userVal=JS_GetPropertyStr(ctx, argv[0], "user");
    scope.addValue(userVal);
    auto passVal=JS_GetPropertyStr(ctx, argv[0], "pass");
    scope.addValue(passVal);
    auto dbVal=JS_GetPropertyStr(ctx, argv[0], "db");
    scope.addValue(dbVal);
    auto portVal=JS_GetPropertyStr(ctx, argv[0], "port");
    scope.addValue(portVal);
    auto socketVal=JS_GetPropertyStr(ctx, argv[0], "socket");
    scope.addValue(portVal);
    std::string socket,host,user,pass,db,port;

    if(JS_IsString(hostVal) )  host = scope.toStdStringView(hostVal);
    else host="NULL";

    if(JS_IsString(portVal) )  port = scope.toStdStringView(portVal);
    else port="NULL";

    if(JS_IsString(userVal) )  user = scope.toStdStringView(userVal);
    else user="NULL";

    if(JS_IsString(passVal) )  pass = scope.toStdStringView(passVal);
    else pass="NULL";

    if(JS_IsString(socketVal) )  socket = scope.toStdStringView(socketVal);
    else socket="NULL";
    MYSQL *conn = mysql_init(NULL);
    if (!conn) return JS_ThrowReferenceError(ctx,"mysql_init failed");

    if (!mysql_real_connect(conn,
                            host!="NULL"?host.c_str():NULL,
                            user!="NULL"?user.c_str():NULL,
                            pass!="NULL"?pass.c_str():NULL,
                            db!="NULL"?db.c_str():NULL,
                            port!="NULL"?atoi(port.c_str()):0,
                            socket!="NULL"? socket.c_str():NULL
                            , 0)) {
        JSValue err = JS_ThrowInternalError(ctx, "MySQL connect error: %s", mysql_error(conn));
        mysql_close(conn);
        return err;
    }

    /*





    JSMySQLConnection *c = new JSMySQLConnection();
    c->conn = conn;

    JSValue obj = JS_NewObjectClass(ctx, 0);
    JS_SetOpaque(obj, c);
    JS_SetPropertyStr(ctx, obj, "query", JS_NewCFunction(ctx, js_query, "query", 1));

    */
    obj = JS_NewObjectClass(ctx, js_mysql_class_id);
    MySQLConnection *s = new MySQLConnection;
    s->conn = conn;

    // if (!mysql_real_connect(s->conn, host, user, password, db, port, NULL, 0)) {
    //     JS_ThrowInternalError(ctx, "MySQL connection failed: %s", mysql_error(s->conn));
    //     js_free(ctx, s);
    //     return JS_EXCEPTION;
    // }

    JS_SetOpaque(obj, s);
    return obj;
}
static JSValue js_mysql_connect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSScope<10,10>  scope(ctx);
    JSValue obj = JS_UNDEFINED;

    if(!JS_IsObject(argv[0])) {
        return JS_ThrowTypeError(ctx, "Expected (Object )");
    }
    auto hostVal=JS_GetPropertyStr(ctx, argv[0], "host");
    scope.addValue(hostVal);
    auto userVal=JS_GetPropertyStr(ctx, argv[0], "user");
    scope.addValue(userVal);
    auto passVal=JS_GetPropertyStr(ctx, argv[0], "password");
    scope.addValue(passVal);
    auto dbVal=JS_GetPropertyStr(ctx, argv[0], "db");
    scope.addValue(dbVal);
    auto portVal=JS_GetPropertyStr(ctx, argv[0], "port");
    scope.addValue(portVal);
    auto socketVal=JS_GetPropertyStr(ctx, argv[0], "socket");
    scope.addValue(portVal);
    std::string socket,host,user,pass,db;
    uint32_t port=0;

    if(JS_IsString(hostVal) )  host = scope.toStdStringView(hostVal);
    else host="NULL";

    if(JS_IsNumber(portVal) )
    {
        if(JS_ToUint32(ctx, &port,portVal)<0) return JS_ThrowTypeError(ctx,"Invalid port value, expected a number");

    }

    if(JS_IsString(userVal) )  user = scope.toStdStringView(userVal);
    else user="NULL";

    if(JS_IsString(passVal) )  pass = scope.toStdStringView(passVal);
    else pass="NULL";

    if(JS_IsString(dbVal) )  db = scope.toStdStringView(dbVal);
    else db="NULL";

    if(JS_IsString(socketVal) )  socket = scope.toStdStringView(socketVal);
    else socket="NULL";
    MYSQL *conn = mysql_init(NULL);
    if (!conn) return JS_ThrowReferenceError(ctx,"mysql_init failed");

    if (!mysql_real_connect(conn,
                            host!="NULL"?host.c_str():NULL,
                            user!="NULL"?user.c_str():NULL,
                            pass!="NULL"?pass.c_str():NULL,
                            db!="NULL"?db.c_str():NULL,
                            port,
                            socket!="NULL"? socket.c_str():NULL
                            , 0)) {
        JSValue err = JS_ThrowInternalError(ctx, "MySQL connect error: %s", mysql_error(conn));
        mysql_close(conn);
        return err;
    }

    obj = JS_NewObjectClass(ctx, js_mysql_class_id);
    MySQLConnection *s = new MySQLConnection;
    s->conn = conn;

    JS_SetOpaque(obj, s);
    return obj;

}

static JSValue js_mysql_query(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSScope <10,10> scope(ctx);

    MySQLConnection *s = (MySQLConnection *)JS_GetOpaque2(ctx, this_val, js_mysql_class_id);
    if (!s) return JS_EXCEPTION;

    std::string  query(scope.toStdStringView(argv[0]));

    if (mysql_query(s->conn, query.c_str())) {
        JS_ThrowInternalError(ctx, "Query failed: %s", mysql_error(s->conn));
        return JS_EXCEPTION;
    }

    MYSQL_RES *res = mysql_store_result(s->conn);
    JSValue arr = JS_NewArray(ctx);
    if (res) {
        int num_fields = mysql_num_fields(res);
        MYSQL_ROW row;
        int i = 0;
        while ((row = mysql_fetch_row(res))) {
            JSValue obj = JS_NewObject(ctx);
            MYSQL_FIELD *fields = mysql_fetch_fields(res);
            for (int j = 0; j < num_fields; j++) {
                JS_SetPropertyStr(ctx, obj, fields[j].name, JS_NewString(ctx, row[j] ? row[j] : "NULL"));
            }
            JS_SetPropertyUint32(ctx, arr, i++, obj);
        }
        mysql_free_result(res);
    }

    return arr;
}

static const JSCFunctionListEntry js_mysql_proto[] = {
    JS_CFUNC_DEF("query", 1, js_mysql_query),
};
#define countof(x) (sizeof(x) / sizeof((x)[0]))

static int js_init_mysql(JSContext *ctx, JSValue global)
{
    JS_NewClassID(&js_mysql_class_id);

    JSClassDef class_def = {
        "MySQLConnection",
        .finalizer = js_mysql_finalizer,
    };
    JS_NewClass(JS_GetRuntime(ctx), js_mysql_class_id, &class_def);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_mysql_proto, countof(js_mysql_proto));

    JSValue ctor = JS_NewCFunction2(ctx, js_mysql_ctor, "mysql", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetClassProto(ctx, js_mysql_class_id, proto);

    JSValue db_obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, db_obj, "mysql", ctor);


    JS_SetPropertyStr(ctx, db_obj, "mysql_connect", JS_NewCFunction(ctx, js_mysql_connect, "mysql_connect", 1));

    JS_SetPropertyStr(ctx, global, "db", db_obj);

    return 0;
}
int js_init_module_mysql(JSContext *ctx)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSScope <10,10> scope(ctx);
    scope.addValue(global);
    // return
    js_init_mysql(ctx, global);
    return 0;
}
