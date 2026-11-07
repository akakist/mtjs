#!/usr/local/bin/mtjs
import * as std from "std";
const GREEN = "\x1b[32m";
const RED = "\x1b[31m";
const RESET = "\x1b[0m";
function errStr(e) {
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}
const tests = [
    "chk-address.js",
    "chk-get-user-nonce.js",
    "chk-mint.js",
    "chk-rollback.js",
    "chk-submit-cb.js",
    "chk-subscribe.js",
    "chk-subscribe-unsubscribe.js",
    "chk-state.js",
    "chk-state1.js",
    "chk-t14-values.js",
    "chk-t15-contract-data.js",
    "chk-t9-node-info.js",
    "chk-block-failed.js",
    "chk-block-get.js",
    "chk-block-happy.js",
    "chk-block-timeout.js",
    "chk-caller.js",
    "chk-delete-granule.js",
    "chk-ttl.js",
    "chk-gym-live.js",
    "chk-gym-buy.js",
    "chk-gym-emits.js",
    "chk-gym-rollback.js",
    "chk-gym-interrupt.js",
    "chk-gym-2nodes.js",
    "chk-gym-balance.js",
    "chk-vote-all.js",
];
(async () => {
    console.log("=== B_RUN-ALL ===");
    console.log("tests: " + tests.length);
    const failed = [];
    for (const t of tests) {
        console.log("\n===== " + t + " =====");
        try {
            await mtjs.asyncExecute("./" + t);
            console.log(GREEN + "✅ " + t + " OK" + RESET);
        }
        catch (e) {
            const es = errStr(e);
            failed.push(t + " — " + es);
            console.log(RED + "❌ " + t + " FAIL: " + es + RESET);
        }
    }
    console.log("\n--- summary ---");
    for (const f of failed)
        console.log("  ❌ " + f);
    if (failed.length === 0) {
        console.log(GREEN + "DONE" + RESET);
        std.exit(0);
    }
    else {
        console.log(RED + "FAIL (" + failed.length + "/" + tests.length + ")" + RESET);
        std.exit(1);
    }
})().catch((e) => {
    console.log(RED + "ERR: " + errStr(e) + RESET);
    std.exit(2);
});
