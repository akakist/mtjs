#!/usr/local/bin/mtjs
"use strict";
mtjs.asyncExecute("ls |./stdin.js").then((data) => {
    console.log("job done");
});
