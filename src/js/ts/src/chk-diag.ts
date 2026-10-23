#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
function errStr(e: any): string { return (e && typeof e==="object" && ("errcode" in e||"errstr" in e)) ? "errcode="+e.errcode+" errstr="+e.errstr : String(e); }

(async () => {
    // 1) get_values — GrainReader жив?
    try {
        const v = await mtjs.get_values(node, 3);
        console.log("[diag] get_values found=" + v.found + " fees=" + JSON.stringify(v.fees));
    } catch (e: any) { console.log("[diag] get_values ERR: " + errStr(e)); }

    // 2) get_node_info для всех n0..n9
    for (let i = 0; i < 10; i++) {
        const n = "n" + i;
        try {
            const ni = await mtjs.get_node_info(node, n, 3);
            console.log("[diag] " + n + " found=" + ni.found + " enabled=" + ni.enabled
                        + " stake=" + ni.full_stake + " owner=" + (ni.owner||"").slice(0,8)
                        + " ip=" + (ni.ip||""));
        } catch (e: any) { console.log("[diag] " + n + " ERR: " + errStr(e)); }
    }

    // 3) get_user_nonce для root — нода отвечает?
    try {
        const addr = mtjs.addr_from_pk(std.getenv("k_root_ed_pk")!);
        const ui = await mtjs.get_user_nonce(node, addr, 3);
        console.log("[diag] root nonce=" + ui.nonce + " balance=" + ui.balance);
    } catch (e: any) { console.log("[diag] get_user_nonce ERR: " + errStr(e)); }

    console.log("[diag] DONE");
})().catch((e: any) => console.log("[diag] ERR:", errStr(e)));
setInterval(() => {}, 1000000);
export{}