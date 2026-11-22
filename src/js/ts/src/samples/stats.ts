#!/usr/local/bin/mtjs
// stats.ts — сбор статистики нод: высота + state root.
// Ноды берутся из состояния (mtjs.get_nodes), опрос — по RPC (get_state).
// Куда писать — решает коллбек юзера.
// (mtjs не поддерживает модули: функции — глобальные в скрипте.)

interface NodeStat {
    name: string;
    ip: string;
    height?: number;
    root?: string;
    ts?: number;
    err?: number | string;
}

async function collectStats(node: string, timeout: number): Promise<NodeStat[]> {
    const r: any = await mtjs.get_nodes(node, timeout);
    const out: NodeStat[] = [];
    for (const n of (r.nodes || [])) {
        try {
            const st: any = await mtjs.get_state(n.ip, timeout);
            if (st.errcode) { out.push({ name: n.name, ip: n.ip, err: st.errcode }); continue; }
            out.push({ name: n.name, ip: n.ip, height: st.block_height, root: st.root_hash, ts: Date.now() });
        } catch (e: any) {
            out.push({ name: n.name, ip: n.ip, err: String(e) });
        }
    }
    return out;
}

function watchStats(node: string, periodMs: number, timeout: number, cb: (s: NodeStat[]) => void): void {
    const tick = async () => {
        let s: NodeStat[];
        try { s = await collectStats(node, timeout); }
        catch (e: any) { s = [{ name: "", ip: "", err: String(e) }]; }
        cb(s);
        setTimeout(tick, periodMs);
    };
    tick();
}

// ── пример: вывод в stdout (юзер заменит на свой sink: лог/CSV/БД) ──
watchStats("127.0.0.1:2301", 1000, 2, (stats) => {
    console.log(JSON.stringify(stats));
});
setInterval(() => {}, 1000000);
