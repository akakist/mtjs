#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const CONTRACT_SRC = "/home/bsr/mtjs/src/js/tsi/dist/contracts/del_test.js";

const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk) throw new Error("no root sk/pk");

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
    return BigInt((await mtjs.get_user_nonce(node, addr, 3)).nonce);
}
async function waitNonceGt(addr: string, last: bigint, to = 10): Promise<bigint> {
    const t0 = Date.now();
    while (Date.now() - t0 < to * 1000) {
        const n = await getNonce(addr);
        if (n > last) return n;
        await new Promise<void>(r => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout");
}
async function submitAndWait(sk: string, addr: string, tx: any[], value = 0, gasLimit = 100000) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, priority: 0 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
async function submitAndWaitErr(sk: string, addr: string, tx: any[], value = 0, gasLimit = 100000) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, priority: 0 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}

// wait — get_contract_data found == wantFound (BlockDBStore может применять write позже блока)
async function waitContractDataFound(contract: string, key: string, wantFound: boolean, to = 15): Promise<boolean> {
    const t0 = Date.now();
    while (Date.now() - t0 < to * 1000) {
        try {
            const g = await mtjs.get_contract_data(node, contract, key, 3);
            if (g.found === wantFound) return true;
        } catch (e: any) { /* TBD — retry */ }
        await new Promise<void>(r => setTimeout(() => r(), 200));
    }
    return false;
}

const rootAddr = mtjs.addr_from_pk(rootPk!);

async function main() {
    console.log("=== DELETE GRANULE (node " + node + ") ===");

    // root funded
    const rb = BigInt((await mtjs.get_user_nonce(node, rootAddr, 3)).balance);
    if (rb < 50000000n) {
        if (rb < 200000n) { check(false, "root low — clean db"); return; }
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "root", method: "mint", params: { amount: "100000000" } },
        ], 0, 20000);
        check(block.status(rsp.tx_hash) === "ok", "mint root");
    }

    let src = "";
    try { src = await mtjs.fs.readFile(CONTRACT_SRC); }
    catch (e: any) { check(false, "read src: " + errStr(e)); return; }
    check(src.length > 0, "src read (" + src.length + " bytes)");

    const c = await mtjs.get_contract(node, "del_test", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "root", method: "contract_update", params: { name: "del_test", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update (errstr=" + block.errstr(rsp.tx_hash) + ")");
    } else {
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: "del_test", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }

    // t1: createX → deleteX → x deleted (found=false)
    {
        console.log("\n[t1] createX → deleteX → x deleted");
        const { rsp: r1, block: b1 } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "del_test", method: "createX", params: {} },
        ]);
        check(b1.status(r1.tx_hash) === "ok", "createX ok (errstr=" + b1.errstr(r1.tx_hash) + ")");

        // x exists (retry — RPC может отставать)
        const ok1 = await waitContractDataFound("del_test", "x", true);
        check(ok1, "x exists before delete (retry)");

        const { rsp: r2, block: b2 } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "del_test", method: "deleteX", params: {} },
        ]);
        check(b2.status(r2.tx_hash) === "ok", "deleteX ok (errstr=" + b2.errstr(r2.tx_hash) + ")");

        // x deleted (retry — BlockDBStore может применить write позже)
        const ok2 = await waitContractDataFound("del_test", "x", false);
        check(ok2, "x deleted after block (found=false, retry)");
    }

    // t2: createX → [deleteX + fail] → err → rollback → x found
    {
        console.log("\n[t2] createX → [deleteX + fail] err → x restored (rollback)");
        // убедиться, что x есть (createX — если удалён в t1)
        const g0 = await mtjs.get_contract_data(node, "del_test", "x", 3);
        if (!g0.found) {
            const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
                { contract: "del_test", method: "createX", params: {} },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "createX (t2)");
        }
        const ok1 = await waitContractDataFound("del_test", "x", true);
        check(ok1, "x exists before t2 delete");

        // deleteX + fail в одной TX → err → rollback
        const { rsp: r2, block: b2 } = await submitAndWaitErr(rootSk!, rootAddr, [
            { contract: "del_test", method: "deleteX", params: {} },
            { contract: "del_test", method: "fail", params: {} },
        ]);
        check(b2.status(r2.tx_hash) === "err", "tx err (deleteX + fail)");
        const es = b2.errstr(r2.tx_hash) || "";
        console.log("  errstr:", es);

        // x restored (Rollback — found=true)
        const ok2 = await waitContractDataFound("del_test", "x", true);
        check(ok2, "x restored (rollback, found=true, retry)");
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED + "FAIL" + RESET);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
