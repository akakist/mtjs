#!/usr/local/bin/mtjs
"use strict";
try {
    console.log("env ", process.env.toString());
    console.log("argv ", process.argv);
    console.log("arch ", process.arch);
    console.log("platform ", process.platform);
    console.log("version ", process.version);
    console.log("versions ", JSON.stringify(process.versions));
    console.log("cwd() ", process.cwd());
    console.log("uptime() ", process.uptime());
    console.log("hrtime() ", process.hrtime());
    console.log("title ", process.title);
    console.log("peakMemoryUsage() ", process.peakMemoryUsage());
    console.log("getuid() ", process.getuid());
    console.log("getgid() ", process.getgid());
    process.chdir("/tmp");
    console.log("cwd() ", [process.cwd()]);
    // exit(code?: number): never;
    // abort(): never;
    // cwd(): string;
}
catch (e) {
    console.log(e);
}
