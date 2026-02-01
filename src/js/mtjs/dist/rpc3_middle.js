#!/usr/local/bin/mtjs
"use strict";
try {
    const rpc = mtjs.rpc;
    rpc.listen({ ip: "localhost:8010", ssl: false });
    rpc.on_server("testMethod_c", (data) => {
        const params = data.params;
        const key2 = params["key"] || "default";
        data.sendTo("localhost:8099", "testMethod_m", { key: key2 + " world" });
    });
}
catch (e) {
    console.log(e);
}
