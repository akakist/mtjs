#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";

let rootSk = ""; let rootPk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk) throw new Error("keys_test.json: no users.root.keys[0].sk/pk");
    rootSk = r.sk; rootPk = r.pk;
}

const GREEN = "\x1b[32m"; const RED = "\x1b[31m"; const RESET = "\x1b[0m";
let pass = 0, fail = 0;
function check(c: boolean, m: string) {
    if (c) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + m); }
    else   { fail++; console.log("  " + RED   + "❌" + RESET + " " + m); }
}
function errStr(e: any): string {
    return (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        ? "errcode=" + e.errcode + " errstr=" + e.errstr : String(e);
}

const rootAddr = mtjs.addr_from_name("root");

(async () => {
    await loadKeys();
    console.log("=== ADDRESS (minimal) ===");

    {
        const info = await mtjs.get_address_info(node, rootAddr, 3);
        console.log("  root found:", info.found, "keys:", JSON.stringify(info.keys));
        check(info.found === true, "root found (addr)");
        check(Array.isArray(info.keys) && info.keys.length === 1, "root keys: 1 (ed25519)");
        if (info.keys && info.keys.length > 0) {
            check(info.keys[0].type === 0, "root keys[0].type == 0 (ed25519)");
            check(typeof info.keys[0].keybuffer === "string" && info.keys[0].keybuffer.length === 64,
                  "root keys[0].keybuffer hex (64)");
        }
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED + "FAIL" + RESET);
     std.exit(fail);
})().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};