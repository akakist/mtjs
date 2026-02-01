#!/usr/local/bin/mtjs
"use strict";
async function test() {
    const fs = mtjs.fs;
    const file = await fs.open("test.txt", fs.O_RDWR | fs.O_CREAT, 0o644);
    await file.write("tipa togo sdfasdfdfslflsf\n fk;asdfkh;kladfs;lk");
    await file.close();
}
test().then(() => {
    console.log("ok");
}).catch((e) => console.log(e));
