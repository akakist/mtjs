#!/usr/local/bin/mtjs
"use strict";
// Сампл: подписка на эмиты ноды → HTTP чанкед (клиент получает события в реальном времени).
// Запуск: mtjs emit_http.js   (после tsc -b), затем  curl -N http://127.0.0.1:6012/
const node = "127.0.0.1:2301";
const WATCH = ["bought", "sold"]; // какие события отдавать
const serv = mtjs.http.createServer((req, res) => {
    res.setHeader("Content-Type", "application/x-ndjson");
    mtjs.tx_subscribe(node, (b) => {
        for (const h of b.tx_hashes) {
            const ems = b.emits(h) || [];
            for (const [name, val] of ems) {
                if (WATCH.indexOf(name) < 0)
                    continue;
                res.write(JSON.stringify({ tx: h, event: name, val }) + "\n"); // чанкед
            }
        }
    });
});
serv.listen(6012);
console.log("emit stream on :6012 (WATCH=" + WATCH.join(",") + ")");
setInterval(() => { }, 1000000);
