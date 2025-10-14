#!/usr/local/bin/mtjs
"use strict";

/*
Since RPC can only bind to one port, 
the RPC demonstration is split into multiple files 
that are launched using asyncExecute.

This example demonstrates RPC work with 3 nodes - client, middle, and server.
The client sends a request to the middle node, 
the middle node forwards the request to the server. 
The server makes a reply and the response goes through the middle node back to the client, 
with no dispatching logic on the middle node.

This demonstrates event routing. 
Events can be forwarded between nodes any number of times, 
the route is remembered, and on reply the event follows the reverse route.
*/
try{

    mtjs.asyncExecute("./rpc3_server.js").then((data:any) => {
        console.log("./rpc3_server.js done");
    });
    mtjs.utils.sleep(1);
    mtjs.asyncExecute("./rpc3_middle.js").then((data:any) => {
        console.log("./rpc3_middle.js done");
    });
    mtjs.utils.sleep(1);

    mtjs.asyncExecute("./rpc3_client.js").then((data:any) => {
        console.log("./rpc3_client.js done");
    });
}
catch(e)
{
    console.log(e);
}