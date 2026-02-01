#!/usr/local/bin/mtjs
/*
Since RPC can only bind to one port, the RPC demonstration is split into multiple files that are launched using asyncExecute.

This example demonstrates RPC work with 2 nodes - client and server.
rpc2_client.ts, rpc2_server.ts

*/
    mtjs.asyncExecute("./rpc2_server.js").then(() => {
        console.log("./rpc2_server.js done");
    });


    mtjs.asyncExecute("./rpc2_client.js").then(() => {
        console.log("./rpc2_client.js done");
    });
