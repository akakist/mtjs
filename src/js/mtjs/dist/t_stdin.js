#!/usr/local/bin/mtjs
"use strict";
/*
This example demonstrates reading from the stdin stream and sending it to an emitter.
*/
mtjs.asyncExecute("ls |./stdin.js").then((data) => {
    console.log("job done");
});
