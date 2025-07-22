#!/usr/local/bin/mtjs
"use strict";
mtjs.asyncExecute("./rpc2_server.js").then(() => {
    console.log("./rpc2_server.js done");
});
mtjs.asyncExecute("./rpc2_client.js").then(() => {
    console.log("./rpc2_client.js done");
});
