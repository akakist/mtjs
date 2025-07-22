#!/usr/local/bin/mtjs
"use strict";
try {
    const ee = new mtjs.stream.EventEmitterStream();
    ee.on("data", (data) => {
        console.log("dataJS ", data);
    });
    ee.on("end", () => {
        console.log("end");
    });
    ee.on("error", (err) => {
        console.log("error ", err);
    });
    mtjs.pipe.read_lines("ls -l", ee);
}
catch (e) {
    console.log("Error: ", e);
}
