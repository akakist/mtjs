#!/usr/local/bin/mtjs

    mtjs.asyncExecute("./rpc2_server.js").then(() => {
        console.log("./rpc2_server.js done");
    });


    mtjs.asyncExecute("./rpc2_client.js").then(() => {
        console.log("./rpc2_client.js done");
    });
