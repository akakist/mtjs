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

function randomAddrHex(): string {
    const s = "0123456789abcdef";
    let out = "";
    for (let i = 0; i < 64; i++) out += s[Math.floor(Math.random() * 16)];
    return out;
}

async function main() {
    await loadKeys();
    console.log("=== GET_USER_NONCE (NoCreate, reject unknown) ===");

    const s0 = await mtjs.get_state(node, 3);
    console.log("  state0:", JSON.stringify(s0));
    const acc0 = s0.account_count;

    // t1: root — found: true (genesis)
    {
        console.log("\n[t1] root (genesis) — found: true");
        const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
        console.log("  ui:", JSON.stringify(ui));
        check(ui.found === true, "root found: true");
        check(ui.nonce !== undefined && ui.balance !== undefined, "root nonce/balance present");
    }

    // t2: неизвестный адрес — reject (address not found)
    {
        console.log("\n[t2] unknown — reject");
        const unknown = randomAddrHex();
        try {
            const ui = await mtjs.get_user_nonce(node, unknown, 3);
            check(false, "unknown: expected reject, got " + JSON.stringify(ui));
        } catch (e: any) {
            const es = errStr(e);
            console.log("  rejected:", es);
            check(es.indexOf("address not found") >= 0, "rejected: address not found");
        }
    }

    // t3: несколько неизвестных — account_count не растёт
    {
        console.log("\n[t3] account_count не растёт (NoCreate)");
        for (let i = 0; i < 5; i++) {
            try { await mtjs.get_user_nonce(node, randomAddrHex(), 3); } catch (_) {}
        }
        const s1 = await mtjs.get_state(node, 3);
        console.log("  account_count:", acc0, "->", s1.account_count);
        check(s1.account_count === acc0, "account_count unchanged: " + acc0 + " -> " + s1.account_count);
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED + "FAIL" + RESET);
    std.exit(fail);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
