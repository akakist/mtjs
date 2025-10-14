#!/usr/local/bin/mtjs
"use strict";
/*
This example:

Listens for RPC on port 8099

Starts a web server on port 6012

For incoming HTTP requests, it stores the HTTP response context in a map

Makes an RPC request to port 8099

Receives the response from RPC

Completes the HTTP request with the response

This demonstrates distributed data collection from remote nodes to respond to HTTP requests.

*/
try {
    const http = mtjs.http;
    const rpc = mtjs.rpc;
    rpc.listen({ ip: "localhost:8099", ssl: false });
    rpc.on_server("testMethod", (data) => {
        const cnt = data.params["id"] || 0;
        data.reply("testMethod", { id: cnt, key: "_repl_ any" });
    });
    let responses = new Map;
    let cnt2 = 0;
    const serv = http.createServer((req, res) => {
        cnt2++;
        responses.set(cnt2, res);
        mtjs.rpc.sendTo("localhost:8099", "testMethod", { id: cnt2, key: "hello" });
    });
    serv.listen(6012);
    rpc.on_client("testMethod", (data) => {
        const id = data.params['id'] || 0;
        if (responses.has(id)) {
            responses.get(id).end("<div>received resonse </>");
            responses.delete(id);
        }
    });
    mtjs.asyncExecute("ab -n 1000000 -k  -c 100   http://127.0.0.1:6012/").then((data) => {
        console.log("ab done");
        serv.stop();
        rpc.stop();
    });
}
catch (r) {
    console.log("error in server " + r);
}
