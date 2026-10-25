#!/usr/local/bin/mtjs
"use strict";
/*
This example listens on port 6012 and runs ab (Apache Bench) to load it with requests.
*/
async function run() {
    await mtjs.asyncExecute("wrk -t10 -c400 -d2s  http://127.0.0.1:6012/");
    await mtjs.asyncExecute("ab -n 1000  -c 100   http://127.0.0.1:6012/");
    await mtjs.asyncExecute("ab -n 1000  -c 100 -k  http://127.0.0.1:6012/");
    await mtjs.asyncExecute("npx autocannon -c 10 -d 2 http://127.0.0.1:6012/");
    await mtjs.asyncExecute("hey -n 10000 -c 50  http://127.0.0.1:6012/");
}
try {
    const http = mtjs.http;
    const serv = http.createServer((req, res) => {
        res.end("<div>received resonse </>");
    });
    serv.listen(6012);
    run().then((data) => {
        //        console.log("ab done", data);
        serv.stop();
    });
    //#    mtjs.asyncExecute("ab -n 1000  -c 100   http://127.0.0.1:6012/")
}
catch (e) {
    console.log("error in server " + e);
}
