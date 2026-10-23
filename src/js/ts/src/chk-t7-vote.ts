#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const pk = std.getenv("k_root_ed_pk");
function check(c: boolean, m: string) { console.log(c ? "  ✅ " + m : "  ❌ " + m); }
function errStr(e: any): string { return (e && typeof e==="object" && ("errcode" in e||"errstr" in e)) ? "errcode="+e.errcode+" errstr="+e.errstr : String(e); }
(async () => {
    console.log("[t7] vote TTL: quorum → vote_applied");
    const addr = mtjs.addr_from_pk(pk!);
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const nonce = BigInt(ui.nonce);
    const param = "vote_test_" + String(Date.now());
    const value = "42";
    console.log("  param:", param);
    const tx = [
        { contract: "root", method: "vote", params: { node: "n5", param, value, ttl: "60" } },
        { contract: "root", method: "vote", params: { node: "n6", param, value, ttl: "60" } },
        { contract: "root", method: "vote", params: { node: "n7", param, value, ttl: "60" } },
        { contract: "root", method: "vote", params: { node: "n8", param, value, ttl: "60" } },
        { contract: "root", method: "vote", params: { node: "n9", param, value, ttl: "60" } },
    ];
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk!, nonce.toString(), () => {});
    console.log("  submit:", rsp.err, rsp.err_str);
    if (rsp.err) { console.log("DONE"); return; }
    const b = await mtjs.tx_wait_block(node, rsp.tx_hash, 10);
    console.log("  status:", b.status(rsp.tx_hash));
    const ch = b.children(rsp.tx_hash);
    let applied = false;
    if (ch) for (const k of Object.keys(ch)) {
        const em = (ch as any)[k].emits as any[];
        console.log("  child["+k+"]:", JSON.stringify(em));
        for (const e of em) if (e[0] === "vote_applied") applied = true;
    }
    check(b.status(rsp.tx_hash) === "ok", "status ok");
    check(applied, "vote_applied present");
    console.log("DONE");
})().catch((e: any) => console.log("ERR:", errStr(e)));
setInterval(() => {}, 1000000);
export{}