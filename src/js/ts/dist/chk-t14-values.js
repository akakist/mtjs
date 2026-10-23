#!/usr/local/bin/mtjs
const node = "127.0.0.1:2301";
function check(cond, msg) {
    if (cond)
        console.log("  ✅", msg);
    else
        console.log("  ❌", msg);
}
function errStr(e) {
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}
(async () => {
    console.log("[t14] get_values");
    try {
        const r = await mtjs.get_values(node, 3);
        console.log("  found:", r.found);
        console.log("  fees:", JSON.stringify(r.fees));
        check(r.found === true, "found");
        check(typeof r.fees === "object", "fees is object");
        check(r.fees["mint"] !== undefined, "fees.mint present");
        check(r.fees["transfer"] !== undefined, "fees.transfer present");
        console.log("  fees.vote:", r.fees["vote"]);
        console.log("  validator_min_stake:", r.fees["validator_min_stake"]);
    }
    catch (e) {
        check(false, "get_values: " + errStr(e));
    }
    console.log("DONE");
})().catch((e) => console.log("ERR:", errStr(e)));
setInterval(() => { }, 1000000);
export {};
