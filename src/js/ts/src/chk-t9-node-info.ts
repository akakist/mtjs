#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
function check(cond: boolean, msg: string) {
    if (cond) console.log("  ✅", msg); else console.log("  ❌", msg);
}
function errStr(e: any): string {
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e)) return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}
(async () => {
    console.log("[t9] get_node_info");
    try {
        for (const n of ["n0", "n5", "n9"]) {
            const r = await mtjs.get_node_info(node, n, 3);
            console.log("  " + n + ":", JSON.stringify(r));
            check(r.found === true, n + ".found");
            check(r.name === n, n + ".name");
            check(typeof r.owner === "string" && r.owner.length > 0, n + ".owner");
            check(typeof r.full_stake === "string", n + ".full_stake");
            check(typeof r.enabled === "boolean", n + ".enabled");
            check(typeof r.stakes === "object", n + ".stakes");
        }
        const notFound = await mtjs.get_node_info(node, "n_nonexistent_" + Date.now(), 3);
        check(notFound.found === false, "unknown node → found: false");
    } catch (e: any) { check(false, "get_node_info: " + errStr(e)); }
    console.log("DONE");
})().catch((e: any) => console.log("ERR:", errStr(e)));
setInterval(() => {}, 1000000);
export {};
