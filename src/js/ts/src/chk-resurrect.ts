#!/usr/local/bin/mtjs
// chk-resurrect.ts — удаление гранулы (тумбстоун) и повторное создание на том же месте,
// плюс компакция: живая гранула не должна удаляться, тумбстоун — должен.
import * as std from "std";

const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";

const SRC = `"use strict";
registerMethod("createX", function (p, m) {
    if (getGranule("x")) throw new Error("x exists");
    createGranule("x", { v: p.v }, 100000);
});
registerMethod("deleteX", function (p, m) {
    if (!getGranule("x")) throw new Error("x not exists");
    deleteGranule("x");
});
`;

let rootSk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk) throw new Error("keys_test.json: no users.root.keys[0].sk");
    rootSk = r.sk;
}

const GREEN = "\x1b[32m"; const RED = "\x1b[31m"; const RESET = "\x1b[0m";
let pass = 0; let fail = 0;
function check(c: boolean, m: string) {
    if (c) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + m); }
    else   { fail++; console.log("  " + RED   + "❌" + RESET + " " + m); }
}
function errStr(e: any): string {
    return (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        ? "errcode=" + e.errcode + " errstr=" + e.errstr : String(e);
}

const pending: { [h: string]: { resolve: (b: Block) => void; reject: (e: any) => void; timer: any } } = {};
mtjs.tx_subscribe(node, (b: Block) => {
    for (const h of b.tx_hashes) {
        const p = pending[h];
        if (p) { delete pending[h]; clearTimeout(p.timer); p.resolve(b); }
    }
});
function waitTx(h: string, to: number): Promise<Block> {
    return new Promise<Block>((res, rej) => {
        const timer = setTimeout(() => { delete pending[h]; rej({ errcode: 1, errstr: "timeout" }); }, to * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}
async function getNonce(addr: string): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.nonce);
}
async function getBalance(addr: string): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.balance);
}
async function submitAndWait(tx: any[], value = 0, gasLimit = 100000, retries = 5) {
    for (let attempt = 0; attempt < retries; attempt++) {
        const nonce = await getNonce(rootAddr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: rootAddr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk, nonce.toString(), () => {});
        if (rsp.err) {
            if (attempt + 1 < retries) { await new Promise(r => setTimeout(r, 300)); continue; }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, 15);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (attempt + 1 < retries) { await new Promise(r => setTimeout(r, 300)); continue; }
            throw new Error("invalid nonce: " + es);
        }
        return { rsp, block: b };
    }
    throw new Error("submitAndWait: retries exhausted");
}
function childEmits(block: Block, txHash: string): { [seq: string]: any[] } {
    const ch = block.children(txHash) as any;
    const out: { [seq: string]: any[] } = {};
    if (ch) for (const k of Object.keys(ch)) out[k] = ch[k].emits;
    return out;
}
function hasEmit(em: { [seq: string]: any[] }, name: string): any {
    for (const k of Object.keys(em)) for (const e of em[k]) {
        if (e[0] !== name) continue;
        let v: any = e[1];
        if (typeof v === "string") { try { v = JSON.parse(v); } catch (_) { return {}; } }
        return v || {};
    }
    return null;
}
async function xState(): Promise<{ found: boolean; v?: any }> {
    const g: any = await mtjs.get_contract_data(node, "resurrect_test", "x", 3);
    if (!g.found) return { found: false };
    let v: any = null;
    try { v = JSON.parse(g.value); } catch (_) { }
    return { found: true, v };
}
async function waitX(wantFound: boolean, to = 15): Promise<boolean> {
    const t0 = Date.now();
    while (Date.now() - t0 < to * 1000) {
        try { if ((await xState()).found === wantFound) return true; } catch (e: any) { }
        await new Promise<void>(r => setTimeout(() => r(), 200));
    }
    return false;
}
const rootAddr = mtjs.addr_from_name("root");

async function compact(label: string) {
    const { rsp, block } = await submitAndWait([
        { contract: "root", method: "compact", params: { retention: "1", max_items: 1000 } },
    ], 0, 200000);
    const st = block.status(rsp.tx_hash);
    const em = hasEmit(childEmits(block, rsp.tx_hash), "compact") || {};
    console.log("  " + label + ": status=" + st + " deleted=" + em.deleted + " complete=" + em.complete + " gen=" + em.gen);
    return { st, em };
}

async function main() {
    await loadKeys();
    console.log("=== RESURRECT (node " + node + ") ===");

    const rb = await getBalance(rootAddr);
    if (rb < 50000000n) {
        if (rb < 200000n) { check(false, "root low — clean db"); return; }
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "mint", params: { amount: "100000000" } },
        ], 0, 20000);
        check(block.status(rsp.tx_hash) === "ok", "mint root");
    }

    const c = await mtjs.get_contract(node, "resurrect_test", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "contract_update", params: { name: "resurrect_test", src: SRC } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update (errstr=" + block.errstr(rsp.tx_hash) + ")");
    } else {
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "contract_deploy", params: { name: "resurrect_test", src: SRC, ttl: 100000 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
    if ((await xState()).found) {
        await submitAndWait([{ contract: "resurrect_test", method: "deleteX", params: {} }]);
    }

    {
        console.log("\n[t1] create(v=1) → deleteX → create(v=2)");
        const { rsp: r1, block: b1 } = await submitAndWait(
            [{ contract: "resurrect_test", method: "createX", params: { v: 1 } }]);
        check(b1.status(r1.tx_hash) === "ok", "create v=1 ok (errstr=" + b1.errstr(r1.tx_hash) + ")");
        check(await waitX(true), "x exists");

        const { rsp: r2, block: b2 } = await submitAndWait(
            [{ contract: "resurrect_test", method: "deleteX", params: {} }]);
        check(b2.status(r2.tx_hash) === "ok", "deleteX ok (errstr=" + b2.errstr(r2.tx_hash) + ")");
        check(await waitX(false), "x deleted (found=false)");

        const { rsp: r3, block: b3 } = await submitAndWait(
            [{ contract: "resurrect_test", method: "createX", params: { v: 2 } }]);
        check(b3.status(r3.tx_hash) === "ok", "create заново ok (errstr=" + b3.errstr(r3.tx_hash) + ")");
        check(await waitX(true), "x снова существует");
        const st = await xState();
        check(!!st.v && st.v.v === 2, "значение новое (v=" + (st.v ? st.v.v : "?") + ", ждали 2)");
    }

    {
        console.log("\n[t2] компакция: живая гранула не удаляется");
        const { st } = await compact("compact");
        check(st === "ok", "compact status ok");
        await new Promise<void>(r => setTimeout(() => r(), 300));
        const s = await xState();
        check(s.found, "x жив после компакции");
        check(!!s.v && s.v.v === 2, "значение всё ещё 2");
    }

    {
        console.log("\n[t3] удалить → компакция → x отсутствует, блоки идут");
        const { rsp: r1, block: b1 } = await submitAndWait(
            [{ contract: "resurrect_test", method: "deleteX", params: {} }]);
        check(b1.status(r1.tx_hash) === "ok", "deleteX ok");
        check(await waitX(false), "x deleted");
        const { st } = await compact("compact");
        check(st === "ok", "compact status ok");
        await new Promise<void>(r => setTimeout(() => r(), 300));
        check(!(await xState()).found, "x отсутствует после чистки тумбстоуна");
        const { rsp: r2, block: b2 } = await submitAndWait(
            [{ contract: "resurrect_test", method: "createX", params: { v: 3 } }]);
        check(b2.status(r2.tx_hash) === "ok", "сеть жива: create(v=3) ok");
        check(await waitX(true), "x снова создан");
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    console.log(fail === 0 ? GREEN + "DONE" + RESET : RED + "FAIL" + RESET);
    std.exit(fail);
}
main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
