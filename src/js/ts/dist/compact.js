#!/usr/local/bin/mtjs
// compact.ts — РУЧНОЙ запуск компакции состояния.
//
// Гоняет батчи tx "compact" у root-контракта, пока состояние не будет
// вычищено до порога (emit complete=1).
//
// Env:
//   NODE            адрес ноды (по умолчанию 127.0.0.1:2301)
//   MTJS_KEYS_PATH  путь к ключам (по умолчанию keys_test.json)
//   RETENTION_DAYS  сколько дней тумбстоунов держать (по умолчанию 180);
//                   cutoff = block_timestamp - retention считается внутри tx
//   MAX_ITEMS       максимум удалений на батч (по умолчанию 1000)
//   MAX_BATCHES     максимум батчей за запуск (0 = без лимита)
//   GAS_LIMIT       лимит газа на tx (по умолчанию 10000000)
import * as std from "std";
const node = std.getenv("NODE") || "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
const RETENTION_DAYS = parseInt(std.getenv("RETENTION_DAYS") || "180", 10);
const MAX_ITEMS = parseInt(std.getenv("MAX_ITEMS") || "1000", 10);
const MAX_BATCHES = parseInt(std.getenv("MAX_BATCHES") || "0", 10);
const GAS_LIMIT = parseInt(std.getenv("GAS_LIMIT") || "10000000", 10);
let rootSk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk)
        throw new Error(KEYS_PATH + ": no users.root.keys[0].sk");
    rootSk = r.sk;
}
const rootAddr = mtjs.addr_from_name("root");
const pending = {};
mtjs.tx_subscribe(node, (b) => {
    for (const h of b.tx_hashes) {
        const p = pending[h];
        if (p) {
            delete pending[h];
            clearTimeout(p.timer);
            p.resolve(b);
        }
    }
});
function waitTx(h, to) {
    return new Promise((res, rej) => {
        const timer = setTimeout(() => { delete pending[h]; rej({ errcode: 1, errstr: "timeout: " + h }); }, to * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}
async function getNonce(addr) {
    return BigInt((await mtjs.get_user_nonce(node, addr, 3)).nonce);
}
async function submitAndWait(tx, gasLimit) {
    for (let attempt = 0; attempt < 5; attempt++) {
        const nonce = await getNonce(rootAddr);
        const req = { tx, nonce: nonce.toString(), value: 0, gasLimit, priority: 0, sender: rootAddr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk, nonce.toString(), () => { });
        if (rsp.err) {
            if (attempt + 1 < 5) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, 30);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (attempt + 1 < 5) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("invalid nonce: " + es);
        }
        return { rsp, block: b };
    }
    throw new Error("submitAndWait: retries exhausted");
}
function compactEmit(block, txHash) {
    const ch = block.children(txHash);
    let out = null;
    if (ch)
        for (const k of Object.keys(ch)) {
            for (const e of ch[k].emits) {
                if (e[0] !== "compact")
                    continue;
                let v = e[1];
                if (typeof v === "string") {
                    try {
                        v = JSON.parse(v);
                    }
                    catch (_) {
                        v = null;
                    }
                }
                if (v)
                    out = v;
            }
        }
    return out;
}
async function main() {
    await loadKeys();
    const retention = BigInt(RETENTION_DAYS) * 86400n * 1000000n; // мкс
    if (retention <= 0n)
        throw new Error("bad retention: " + retention.toString());
    console.log("compact: node=" + node + " retention=" + RETENTION_DAYS + "d (" + retention.toString() + " mks)" +
        " max_items=" + MAX_ITEMS);
    let totalDeleted = 0, batches = 0, gen = null;
    for (;;) {
        if (MAX_BATCHES > 0 && batches >= MAX_BATCHES) {
            console.log("compact: stop by MAX_BATCHES=" + MAX_BATCHES + " (не всё вычищено)");
            break;
        }
        batches++;
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "compact", params: { retention: retention.toString(), max_items: MAX_ITEMS } },
        ], GAS_LIMIT);
        const st = block.status(rsp.tx_hash);
        if (st !== "ok")
            throw new Error("compact tx failed: " + block.errstr(rsp.tx_hash));
        const em = compactEmit(block, rsp.tx_hash) || {};
        const deleted = em.deleted ?? 0;
        gen = em.gen ?? gen;
        const cutoff = em.cutoff ?? null;
        totalDeleted += deleted;
        console.log("  batch " + batches + ": deleted=" + deleted + " complete=" + (em.complete ?? "?") +
            " gen=" + gen + (cutoff !== null ? " cutoff=" + cutoff : ""));
        if (em.complete)
            break;
        if (deleted === 0) {
            console.log("compact: 0 deleted, останов");
            break;
        }
    }
    console.log("compact: done, batches=" + batches + ", deleted=" + totalDeleted + ", gen=" + gen);
}
main().then(() => std.exit(0)).catch((e) => {
    console.error("compact: FAILED: " + (e && e.message ? e.message : JSON.stringify(e)));
    std.exit(1);
});
