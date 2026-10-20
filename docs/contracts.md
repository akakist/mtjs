# JSI — контракты и тесты

## Что такое JSI

**JSI** (JavaScript Integer) — детерминированный JS-подобный язык для смарт-контрактов. Только `int64`, синхронно, без `float`/`Date`/`async`/`eval`/`RegExp`. Всё остальное — привычный JS: `let`/`const`, `if`, `for`, функции, объекты, массивы, строки.

## Имя контракта

- **В состоянии и вызовах** — **просто** `<name>` (**без** `contracts/`, **без** `.js`). Ключ гранулы — `<name>/<key>`. `callMethod("<name>", ...)`.
- **Файл** `contracts/<name>.js` — **только** для оффчейн-тестирования. Раннер загружает из `contracts/`, но **имя в рантайме** — просто `<name>`.

## Структура проекта

```
project/
├── contracts/          # файлы контрактов (.js) — для оффчейн-тестирования
│   ├── counter.js      # имя контракта в рантайме: "counter"
│   ├── transfer.js     # имя контракта в рантайме: "transfer"
│   └── escrow.js       # имя контракта в рантайме: "escrow"
├── tx/                 # тесты (.tx.js)
│   ├── counter.test.tx.js
│   ├── transfer.test.tx.js
│   └── escrow.test.tx.js
└── test_runner         # бинарник раннера
```

Раннер запускается из корня (`./test_runner`), читает `tx/`, загружает контракты из `contracts/`.

## Контракт

Контракт — файл `contracts/<name>.js`. Использует **состояние** (гранулы) и **API** раннера.

### Состояние

- **`getGranule(key)`** — читает гранулу **своего контракта** (`<name>/<key>`). Нет — `undefined`.
- **`createGranule(key, value, ttl)`** — создаёт. Уже есть — `TypeError`.
- **`setGranule(key, value)`** — обновляет. Нет — `TypeError`.

Этого **достаточно** для оперирования данными. Ключ **автоматически** получает префикс `<name>/`.

```js
registerMethod("transfer", function(params, msg) {
    let from = getGranule(params.from);
    let to   = getGranule(params.to);
    if (from.balance < params.amount) throw new Error("insufficient");
    from.balance -= params.amount;
    to.balance   += params.amount;
    setGranule(params.from, from);
    setGranule(params.to, to);
    emit("Transfer", { from: params.from, to: params.to, amount: params.amount });
});
```

### Методы и view

- **`registerMethod(name, fn)`** — метод. **Может** писать, эмитить. **Принимает** `(params, msg)`.
- **`registerView(name, fn)`** — view. **Только** читает. **Принимает** `(params)`.

**`msg`** — данные транзакции: `sender`, `value`, `gasLimit`, `blockTimestamp`, `blockHeight`, `nonce`, `chainId`, `txHash`, `txIndex`, `gasUsed`.

### Вызовы между контрактами

- **`callMethod(contract, method, params)`** — вызов **метода** другого контракта.
- **`callView(contract, view, params)`** — вызов **view** другого контракта.

**На себя** — нельзя (вызывай напрямую).

### События

- **`emit(name, value)`** — **только** в методе (не во view). **Атомарно**: упавшая транзакция **не** оставляет событий.

## Тестирование оффчейн

Контракт **тестируется** утилитой `test_runner` **оффчейн** — **без** блокчейна, деплоя, консенсуса. Раннер загружает контракты из `contracts/`, прогоняет тесты из `tx/`, сравнивает состояние и события с ожиданием.

### Формат `.tx.js`

```js
({
    time: 1000,
    initial: { "transfer/g1": { balance: 1000 } },
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "transfer", method: "transfer",
                params: { from: "g1", to: "g2", amount: 100 },
                expect_result: ...,       // опционально
                expect_error: false,       // опционально
                expect_events: [ ... ] }   // опционально
          ] }
    ],
    expected_emits: [ ... ],
    expected: { "transfer/g1": { balance: 900 } }
})
```

- **`initial`** — состояние **до** (полные ключи `<name>/<key>`).
- **`transactions`** — массив транзакций, **каждая** с `msg` и `commands`.
- **`commands`** — вызовы **методов** (**атомарно** внутри транзакции).
- **`expected_emits`** — все события **за прогон**.
- **`expected`** — состояние **после**.

### Атомарность

Упавшая **команда** (**или** `expect_error: true`) **откатывает всю транзакцию** — гранулы **и** события **как** до.

### Прогон

```bash
./test_runner
```

**PASS**/**FAIL** с диффом:
```
result mismatch in transfer.transfer: expected {"balance":900}, got {"balance":901}
  mismatch at transfer/g1: expected {"balance":900}, got {"balance":901}
  events count mismatch in transfer.transfer: expected 1, got 0
```

## Пример: counter

**`contracts/counter.js`**

```js
registerMethod("increment", function(params, msg) {
    let g = getGranule("count");
    let n = (g ? g.value : 0) + (params.by || 1);
    if (g) setGranule("count", { value: n });
    else   createGranule("count", { value: n }, 0);
    emit("Incremented", { by: params.by || 1, total: n, sender: msg.sender });
    return n;
});

registerView("get", function(params) {
    let g = getGranule("count");
    return { value: g ? g.value : 0 };
});
```

**`tx/counter.test.tx.js`**

```js
({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "counter", method: "increment",
                params: { by: 5 }, expect_result: 5,
                expect_events: [
                    { contract: "counter", name: "Incremented",
                      value: { by: 5, total: 5, sender: "alice" } }
                ] }
          ] }
    ],
    expected_emits: [
        { contract: "counter", name: "Incremented",
          value: { by: 5, total: 5, sender: "alice" } }
    ],
    expected: { "counter/count": { value: 5 } }
})
```

## Пример: transfer (балансы, атомарность)

**`contracts/transfer.js`**

```js
registerMethod("transfer", function(params, msg) {
    let from = getGranule(params.from);
    let to   = getGranule(params.to);
    if (!from) throw new Error("from not found");
    if (!to)   throw new Error("to not found");
    if (from.balance < params.amount) throw new Error("insufficient balance");
    from.balance -= params.amount;
    to.balance   += params.amount;
    setGranule(params.from, from);
    setGranule(params.to, to);
    emit("Transfer", { from: params.from, to: params.to,
                       amount: params.amount, sender: msg.sender });
});

registerView("balanceOf", function(params) {
    let g = getGranule(params.address);
    return { balance: g ? g.balance : 0 };
});
```

**`tx/transfer.basic.tx.js`**

```js
({
    time: 1000,
    initial: {
        "transfer/g1": { balance: 1000 },
        "transfer/g2": { balance: 500 }
    },
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "transfer", method: "transfer",
                params: { from: "g1", to: "g2", amount: 100 },
                expect_events: [
                    { contract: "transfer", name: "Transfer",
                      value: { from: "g1", to: "g2", amount: 100, sender: "alice" } }
                ] }
          ] }
    ],
    expected_emits: [
        { contract: "transfer", name: "Transfer",
          value: { from: "g1", to: "g2", amount: 100, sender: "alice" } }
    ],
    expected: {
        "transfer/g1": { balance: 900 },
        "transfer/g2": { balance: 600 }
    }
})
```

**`tx/transfer.atomic.tx.js`** — атомарность

```js
({
    time: 1000,
    initial: {
        "transfer/g1": { balance: 1000 },
        "transfer/g2": { balance: 500 }
    },
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "transfer", method: "transfer",
                params: { from: "g1", to: "g2", amount: 100 } },
              { contract: "transfer", method: "transfer",
                params: { from: "g1", to: "g2", amount: 99999 },
                expect_error: true }
          ] }
    ],
    expected_emits: [],
    expected: {
        "transfer/g1": { balance: 1000 },
        "transfer/g2": { balance: 500 }
    }
})
```

**`tx/transfer.multisender.tx.js`** — взаимодействие адресов

```js
({
    time: 1000,
    initial: {
        "transfer/g1": { balance: 1000 },
        "transfer/g2": { balance: 500 },
        "transfer/g3": { balance: 0 }
    },
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [ { contract: "transfer", method: "transfer",
                        params: { from: "g1", to: "g2", amount: 100 } } ] },
        { msg: { sender: "bob", blockTimestamp: 2000 },
          commands: [ { contract: "transfer", method: "transfer",
                        params: { from: "g2", to: "g3", amount: 50 } } ] },
        { msg: { sender: "carol", blockTimestamp: 3000 },
          commands: [ { contract: "transfer", method: "transfer",
                        params: { from: "g3", to: "g1", amount: 25 } } ] }
    ],
    expected_emits: [
        { contract: "transfer", name: "Transfer",
          value: { from: "g1", to: "g2", amount: 100, sender: "alice" } },
        { contract: "transfer", name: "Transfer",
          value: { from: "g2", to: "g3", amount: 50, sender: "bob" } },
        { contract: "transfer", name: "Transfer",
          value: { from: "g3", to: "g1", amount: 25, sender: "carol" } }
    ],
    expected: {
        "transfer/g1": { balance: 925 },
        "transfer/g2": { balance: 550 },
        "transfer/g3": { balance: 25 }
    }
})
```

## Пример: callView через метод

**`contracts/observer.js`**

```js
registerMethod("peek", function(params, msg) {
    return callView("transfer", "balanceOf", params);
});

registerView("badView", function(params) {
    setGranule("x", { foo: 1 });   // TypeError: not allowed in view
    return null;
});

registerMethod("callBad", function(params, msg) {
    return callView("observer", "badView", params);
});
```

**`tx/observer.callview.tx.js`**

```js
({
    time: 1000,
    initial: { "transfer/g1": { balance: 1000 } },
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "observer", method: "peek",
                params: { address: "g1" },
                expect_result: { balance: 1000 } }
          ] }
    ],
    expected_emits: [],
    expected: { "transfer/g1": { balance: 1000 } }
})
```

**`tx/observer.badview.tx.js`**

```js
({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "observer", method: "callBad",
                params: {}, expect_error: true }
          ] }
    ],
    expected_emits: [],
    expected: {}
})
```

## Пример: tickets (TTL)

**`contracts/tickets.js`**

```js
registerMethod("createConcert", function(params, msg) {
    createGranule("concert/" + params.id, {
        name: params.name,
        total: params.total,
        sold: 0
    }, params.ttl);
    emit("ConcertCreated", { id: params.id, name: params.name });
});

registerMethod("buyTicket", function(params, msg) {
    let concert = getGranule("concert/" + params.concert);
    if (!concert) throw new Error("concert not found");
    if (concert.sold >= concert.total) throw new Error("sold out");

    let seat = concert.sold;
    createGranule("ticket/" + params.concert + "/" + seat, {
        owner: msg.sender,
        seat: seat,
        concert: params.concert
    }, params.ttl);

    concert.sold += 1;
    setGranule("concert/" + params.concert, concert);

    emit("TicketSold", { concert: params.concert, seat: seat, owner: msg.sender });
});

registerView("ticketOf", function(params) {
    return getGranule("ticket/" + params.concert + "/" + params.seat);
});
```

**`tx/tickets.sell.tx.js`**

```js
({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "organizer", blockTimestamp: 1000 },
          commands: [
              { contract: "tickets", method: "createConcert",
                params: { id: "rock2026", name: "Rock 2026", total: 3, ttl: 86400 } }
          ] },
        { msg: { sender: "alice", blockTimestamp: 1100 },
          commands: [
              { contract: "tickets", method: "buyTicket",
                params: { concert: "rock2026", ttl: 86400 } }
          ] },
        { msg: { sender: "bob", blockTimestamp: 1200 },
          commands: [
              { contract: "tickets", method: "buyTicket",
                params: { concert: "rock2026", ttl: 86400 } }
          ] }
    ],
    expected_emits: [
        { contract: "tickets", name: "ConcertCreated",
          value: { id: "rock2026", name: "Rock 2026" } },
        { contract: "tickets", name: "TicketSold",
          value: { concert: "rock2026", seat: 0, owner: "alice" } },
        { contract: "tickets", name: "TicketSold",
          value: { concert: "rock2026", seat: 1, owner: "bob" } }
    ],
    expected: {
        "tickets/concert/rock2026": { name: "Rock 2026", total: 3, sold: 2 },
        "tickets/ticket/rock2026/0": { owner: "alice", seat: 0, concert: "rock2026" },
        "tickets/ticket/rock2026/1": { owner: "bob", seat: 1, concert: "rock2026" }
    }
})
```

## Пример: escrow (арбитр, таймаут)

**`contracts/escrow.js`**

```js
registerMethod("deposit", function(params, msg) {
    createGranule("deal/" + params.id, {
        buyer: msg.sender,
        seller: params.seller,
        arbiter: params.arbiter,
        amount: params.amount,
        status: "active",
        deadline: msg.blockTimestamp + params.timeout
    }, params.ttl);
    emit("Deposited", { id: params.id, buyer: msg.sender, amount: params.amount });
});

registerMethod("release", function(params, msg) {
    let deal = getGranule("deal/" + params.id);
    if (!deal) throw new Error("deal not found");
    if (deal.status != "active") throw new Error("not active");
    if (msg.sender != deal.buyer && msg.sender != deal.arbiter)
        throw new Error("not authorized");
    deal.status = "released";
    setGranule("deal/" + params.id, deal);
    emit("Released", { id: params.id, to: deal.seller, amount: deal.amount });
});
```

**`tx/escrow.release.tx.js`**

```js
({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "escrow", method: "deposit",
                params: { id: "deal1", seller: "bob", arbiter: "carol",
                          amount: 500, timeout: 86400, ttl: 86400 } }
          ] },
        { msg: { sender: "bob", blockTimestamp: 2000 },
          commands: [
              { contract: "escrow", method: "release",
                params: { id: "deal1" } }
          ] }
    ],
    expected_emits: [
        { contract: "escrow", name: "Deposited",
          value: { id: "deal1", buyer: "alice", amount: 500 } },
        { contract: "escrow", name: "Released",
          value: { id: "deal1", to: "bob", amount: 500 } }
    ],
    expected: {
        "escrow/deal/deal1": {
            buyer: "alice", seller: "bob", arbiter: "carol",
            amount: 500, status: "released", deadline: 87400
        }
    }
})
```

## Отладка

**Чтение FAIL:**

- `result mismatch in <contract>.<method>: expected X, got Y` — метод вернул не то.
- `mismatch at <key>: expected X, got Y` — гранула не та.
- `events count mismatch` / `event[N] mismatch` — события не те.
- `unexpected exception` — метод упал, а не должен (stderr печатает сообщение).
- `expected error, but got result` — метод не упал, а должен.

**Отладка контракта** — временно добавь `emit` с промежуточными значениями и ожидай его в тесте.

## Что нельзя

`float`/`NaN`/`Infinity`, `Date`, `async`/`Promise`, `eval`/`Function`, `RegExp`, `Proxy`/`Reflect`, `Map`/`Set`, `TypedArray`, `BigInt`, `Symbol`, `Math`, `JSON`, `**`. Гранула **≤ 10 KB**. `undefined` **не** пишется.

## Быстрый справочник

```
registerMethod(name, fn(params, msg))
registerView(name, fn(params))
createGranule(key, value, ttl)    // создаёт, уже есть -> TypeError
setGranule(key, value)             // обновляет, нет -> TypeError
getGranule(key)                    // читает, нет -> undefined
emit(name, value)                  // только в методе
callMethod(contract, method, params)
callView(contract, view, params)
```

**`msg`:** `sender`, `value`, `gasLimit`, `blockTimestamp`, `blockHeight`, `nonce`, `chainId`, `txHash`, `txIndex`, `gasUsed`.

**`.tx.js`:** `time`, `initial`, `transactions: [{msg, commands}]`, `expected_emits`, `expected`.

**Имя контракта:** в состоянии и вызовах — `<name>`. Файл — `contracts/<name>.js` (только для оффчейн-тестов).

**Прогон:** `./test_runner` → **PASS**/**FAIL** с диффом.

---

