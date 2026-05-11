#!/usr/local/bin/mtjs
"use strict";
try {
    const http = mtjs.http;
    const serv = http.createServer((req, res) => {
        res.end("<div>received resonse </>");
    });
    serv.listen(6012);
    mtjs.asyncExecute("wrk -t100 -c400 -d10s   http://127.0.0.1:6012/").then((data) => {
        console.log("ab done", data);
        serv.stop();
    });

}
catch (e) {
    console.log("error in server " + e);
}
