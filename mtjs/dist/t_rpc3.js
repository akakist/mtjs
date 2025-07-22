#!/usr/local/bin/mtjs
"use strict";
try {
    mtjs.asyncExecute("./rpc3_server.js").then((data) => {
        console.log("./rpc3_server.js done");
    });
    mtjs.utils.sleep(1);
    mtjs.asyncExecute("./rpc3_middle.js").then((data) => {
        console.log("./rpc3_middle.js done");
    });
    mtjs.utils.sleep(1);
    mtjs.asyncExecute("./rpc3_client.js").then((data) => {
        console.log("./rpc3_client.js done");
    });
}
catch (e) {
    console.log(e);
}
