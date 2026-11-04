#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
function errStr(e: any): string { return (e && typeof e==="object" && ("errcode" in e||"errstr" in e)) ? "errcode="+e.errcode+" errstr="+e.errstr : String(e); }

(async () => {
    try {
        const v = await mtjs.get_values(node, 3);
        console.log("[diag] get_values found=" + v.found + " fees=" + JSON.stringify(v.fees));
    } catch (e: any) { console.log("[diag] get_values ERR: " + errStr(e)); }

    for (let i = 0; i < 10; i++) {
        const n = "n" + i;
        try {
            const ni = await mtjs.get_node_info(node, n, 3);
            console.log("[diag] " + n + " found=" + ni.found + " enabled=" + ni.enabled
                        + " stake=" + ni.full_stake + " owner=" + (ni.owner||"").slice(0,8)
                        + " ip=" + (ni.ip||""));
        } catch (e: any) { console.log("[diag] " + n + " ERR: " + errStr(e)); }
    }

    try {
        const addr = mtjs.addr_from_name("root");
        const ui = await mtjs.get_user_nonce(node, addr, 3);
        console.log("[diag] root nonce=" + ui.nonce + " balance=" + ui.balance);
    } catch (e: any) { console.log("[diag] get_user_nonce ERR: " + errStr(e));  std.exit(1);}

    console.log("[diag] DONE");
    std.exit(0);
})().catch((e: any) => console.log("[diag] ERR:", errStr(e)));
setInterval(() => {}, 1000000);
export{}

