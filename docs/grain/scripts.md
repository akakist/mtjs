# Скрипты и HTTP-интеграция

Скрипт — JS-файл, исполняемый бинарём `mtjs` (QuickJS). Шебанг `#!/usr/local/bin/mtjs`.
API клиента/скриптов — объект `mtjs` (типы: `mtjs.d.ts`).

## Основные вызовы mtjs

```js
mtjs.tx_submit(node, timeout, req_json, sk, nonce, cb)   // отправить tx
mtjs.get_state(node, timeout)        // StateInfo (block_height, total_supply, ...)
mtjs.get_block(node, timeout)        // Block (block_hash, tx_hashes, block_timestamp)
mtjs.get_contract_data(node, c, key, timeout)   // гранула контракта
mtjs.get_user_nonce(node, addr, timeout)
mtjs.tx_subscribe(node, cb)          // подписка на каждый новый блок
```

`Block` (в колбэке `tx_subscribe`): `tx_hashes`, `block_hash`, `block_timestamp`,
`status(tx_hash)`, `emits(tx_hash)`, `children(tx_hash)`.

## HTTP-сервер

Модуль в стиле Node.js:
```js
const serv = mtjs.http.createServer((req, res) => {
    res.end("<div>ok</div>");
});
serv.listen(6012);
```

- `req.is_chunked` — запрос с `Transfer-Encoding: chunked`.
- `res.write(x)` → чанкед-ответ; только `res.end(x)` → `Content-Length`.
- Стримы: `mtjs.stream.*` (`FileReadableStream`, `EventEmitterStream`, …) — см. `docs/grain/streams.md`.

## Стриминг блоков/эмитов через chunked

Подписка `tx_subscribe` даёт каждый блок; его можно отдавать HTTP-клиенту чанками.
Два режима:

- **все блоки** — пишем весь блок (hash, timestamp, tx_hashes);
- **только эмиты** — пишем только события интересующих контрактов.

### Сампл 1: HTTP + chunked (echo)

```js
#!/usr/local/bin/mtjs
"use strict";
const serv = mtjs.http.createServer((req, res) => {
    if (req.is_chunked) {
        let s = new mtjs.stream.EventEmitterStream;
        s.on("data", (d) => res.write(d));
        s.on("end", () => res.end());
        req.stream = s;
    } else {
        res.end("not chunked");
    }
});
serv.listen(6012);
setInterval(() => {}, 1000000);
```

### Сампл 2: стрим всех блоков

```js
#!/usr/local/bin/mtjs
"use strict";
const node = "127.0.0.1:2301";
const serv = mtjs.http.createServer((req, res) => {
    res.write("[");                      // chunked: открываем массив
    let first = true;
    mtjs.tx_subscribe(node, (b) => {
        const j = JSON.stringify({ hash: b.block_hash, ts: b.block_timestamp, txs: b.tx_hashes });
        res.write((first ? "" : ",") + j);
        first = false;
    });
});
serv.listen(6012);
setInterval(() => {}, 1000000);
```

### Сампл 3: стрим только эмитов (фильтр)

```js
#!/usr/local/bin/mtjs
"use strict";
const node = "127.0.0.1:2301";
const WATCH = ["market"];               // интересующие контракты
const serv = mtjs.http.createServer((req, res) => {
    mtjs.tx_subscribe(node, (b) => {
        for (const h of b.tx_hashes) {
            const ems = b.emits(h) || [];
            for (const [name, val] of ems) {
                if (WATCH.indexOf(name) >= 0) {
                    res.write(JSON.stringify({ tx: h, event: name, val }) + "\n");
                }
            }
        }
    });
});
serv.listen(6012);
setInterval(() => {}, 1000000);
```

> В сампле 3 сервер отдаёт **только эмиты** выбранных контрактов (меньше трафика, удобно для
> аналитики: emit → relational DB). В сампле 2 — полные блоки. Оба — чанкед-поток, клиент читает
> по мере поступления, без `Content-Length`.

## Сборка

`cd src/js/ts && ./r.sh` (tsc -b) — компилирует `.ts` → `.js`. Скрипты кладутся в `src/js/ts/dist/`.

## Папка (pipe): чанкед → файл, минуя JS-цикл

Стримы `mtjs.stream.*` — это `Stream` с `read`/`write` и `pipe()` (C++-метод `Stream::pipe`).
Один конец вешаем на HTTP-запрос (`req.stream`), другой — на файл: входящие чанки идут
в файл **напрямую C++→C++**, не проходя главный поток JS.

```js
#!/usr/local/bin/mtjs
"use strict";
const serv = mtjs.http.createServer((req, res) => {
    if (req.is_chunked) {
        // входящие чанки → файл, JS не участвует
        req.stream = new mtjs.stream.FileWriteableStream("/tmp/upload.bin");
        res.end("ok");
    } else {
        res.end("not chunked");
    }
});
serv.listen(6012);
setInterval(() => {}, 1000000);
```

## Стримы (mtjs.stream)

- `FileReadableStream(fn)` / `FileWriteableStream(fn)` — файл.
- `ConstReadableStringStream(s)` / `WriteableStringStream()` — строка.
- `EventEmitterStream` — данные в JS-колбэки (`on("data")`, `on("end")`).
- Методы: `write`, `end`, `read`, `on`.
- `req.stream = <stream>` — чем принимать входящий чанкед-поток (по умолчанию без стрима данные теряются).

## База данных (db.mysql)

В рантайме уже есть MySQL (libmariadb), глобальный объект `db`:

```js
const conn = db.mysql({ host: "127.0.0.1", user: "root", pass: "", db: "test", port: "3306" });
const rows = conn.query("SELECT id, name FROM users");   // массив строк
```

Postgres пока нет — только MySQL.
