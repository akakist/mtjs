#!/usr/local/bin/mtjs
import * as std from "std";
// JSON.stringify с поддержкой BigInt (значения гранул — 128-битные)
function jstr(v) {
    return JSON.stringify(v, (_k, x) => (typeof x === "bigint" ? Number(x) : x));
}
const fake = "00".repeat(32);
mtjs.tx_wait_block("127.0.0.1:2301", fake, 1)
    .then((b) => console.log("UNEXPECTED:", b.block_hash))
    .catch((e) => { console.log("rejected:", jstr(e)); std.exit(0); });
setInterval(() => { }, 1000000);
