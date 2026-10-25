#!/usr/local/bin/mtjs

"use strict";
try{
    mtjs.rpc.listen({ ip: "localhost:8099", ssl: false });
    mtjs.rpc.on_server("testMethod_m", (data:rpcServerRequest) => {
        const params = data.params;
        const key2 = params["key"] || "default";
        data.reply("testMethod_s", { key: key2 + " world SRV" });
    });
}catch(e){console.log(e)}