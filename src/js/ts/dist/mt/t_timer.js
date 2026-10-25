#!/usr/local/bin/mtjs
"use strict";
/*
This example shows the use of a timer.
*/
try {
    const t2 = setInterval((a, b) => {
        console.log("intervalled " + a + "  " + b + t2.toString());
    }, 1000, "hello", "wasya");
    console.log(t2.toString());
    const t = setTimeout((a, b) => {
        console.log("timed " + a + "  " + b);
    }, 2000, "hello", "valya");
    console.log("after set timer");
    const t3 = setInterval((a, b) => {
        console.log("intervalled unref " + a + "  " + b);
    }, 500, "hello", "intervalled unref")
        .unref();
}
catch (e) {
    console.log(e);
}
