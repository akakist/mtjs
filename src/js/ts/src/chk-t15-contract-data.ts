#!/usr/local/bin/mtjs
const node = "127.0.0.1:2301";
function check(cond: boolean, msg: string) {
    if (cond) console.log("  ✅", msg); else console.log("  ❌", msg);
}
function errStr(e: any): string {
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e)) return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}
(async () => {
    console.log("[t15] get_contract_data");
    try {
        const r = await mtjs.get_contract_data(node, "gym", "pass/u0", 3);
        console.log("  found:", r.found, "contract:", r.contract, "key:", r.key);
        check(r.found === false, "gym/pass/u0 not found (no gym contract yet)");
        check(r.contract === "gym", "contract echoed");
        check(r.key === "pass/u0", "key echoed");
    } catch (e: any) { check(false, "get_contract_data: " + errStr(e)); }
    console.log("DONE");
})().catch((e: any) => console.log("ERR:", errStr(e)));
setInterval(() => {}, 1000000);
export {};