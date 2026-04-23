#!/usr/local/bin/mtjs
/*
This example demonstrates reading from the stdin stream and sending it to an emitter.
*/
mtjs.asyncExecute("ls |./stdin.js").then((data:any) => {
    console.log("job done");
});
