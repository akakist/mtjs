# Разработка контрактов GRAIN

Контракт — JS-функции (QuickJS), регистрируемые через `registerMethod` / `registerView` / `registerQuery`.
Три точки входа различаются **правами на запись** и **временем**.

## Три способа регистрации

```js
registerMethod("buy", function (params, msg) {   // мутация, msg = sender/value/gas
    createGranule("order", { id: params.id }, 86400);
    emit("bought", { id: params.id });
});

registerView("price", function (params) {        // только чтение, БЕЗ msg
    return getGranule("price");
});

registerQuery("time", function (params) {        // только чтение, wall-clock
    return now();                                 // время запроса
});
```

| | `method` | `view` | `query` |
|---|---|---|---|
| Запись состояния | ✅ (createGranule/setGranule/emit) | ❌ | ❌ |
| `msg` (sender/value/gas) | ✅ `JsiMsg` | ❌ (анонимно) | ❌ (анонимно) |
| Время `now` | block_timestamp (чейн) | block_timestamp (чейн) | wall-clock (сэмпл на входе) |
| Кто вызывает | транзакция | tx (`callView`) **и** reader | только reader |
| Газ | тратит | — | — |

- **method** — меняет состояние; получает `msg: { sender, value, gasLimit, gasUsed }`; `now` = время блока.
- **view** — чистое чтение из состояния; вызывается и из tx (другой контракт), и из reader;
  `now` = время блока (детерминированно).
- **query** — чтение с wall-clock временем (для «сколько сейчас», IP и т.п.); только из reader.

Внутри: `registerView` кладёт функцию и в `views`, и в `queries` (доступна обоим путям);
`registerQuery` — только в `queries`.

## Окружение JS (важно)

Рантайм QuickJS, только int64. **Запрещены** (grep в `r.sh`, tsc их не ловит):
`Math, JSON, Date, RegExp, Promise, Map, Set, WeakMap, WeakSet, Proxy, Reflect, BigInt, ArrayBuffer, DataView, parseFloat, isNaN, isFinite`.

**Доступны**: `Object, Function, Error, Array, Number, Boolean, String, Symbol, Generator, GeneratorFunction, parseInt, globalThis`.

Арифметика int64: `/` — целочисленное (10/3=3), `+ - *` — с переполнением (→ RangeError).

## Хранение: гранулы

Плоский API:
```js
getGranule(key)                        // JsiGranule | undefined
createGranule(key, value, ttl)         // ttl в секундах (>0, ≤ MAX)
setGranule(key, value)                 // обновить + автопродлить activity
exists_granule(key)
```

Иерархический API (`JsiStorage` — вложенный узел):
```js
let n = get_granule("a/b");            // JsiStorage | null
let c = n.get_granule("c");            // вложенный
create_granule("a/b/c", ttl)           // дерево внутри контракта
```

Удаления контрактом нет: правило — **«выставил TTL, нет активности → удалилось само»** (GC).

## События и межконтрактные вызовы

```js
emit(name, value)                       // событие (сериализуемо)
callMethod(contract, method, params)    // вызвать метод другого контракта
callView(contract, view, params, timeout_sec)
consumeValue(amount) / refundValue(amount) / remainingValue()
```

## Окружение vscode (d.ts)

Типы лежат в `src/js/tsi/src/types/`:
- `jsi.d.ts` — API контрактов (`registerMethod`, `registerView`, `registerQuery`, `getGranule`, `emit`, `JsiMsg`, `JsiStorage`…).
- `mtjs.d.ts` — API клиента/скриптов (`mtjs.tx_submit`, `get_state`, `tx_subscribe`, `Block`…).

В vscode: открыть `src/js/tsi/` (есть `tsconfig.json`), или добавить в свой `tsconfig.json`:
```json
{ "compilerOptions": { "types": [] }, "include": ["src/**/*.ts", "../tsi/src/types/**/*.d.ts"] }
```
Тогда IDE подсказывает имена функций и типы (`JsiMsg`, `JsiStorage`, `JsiGranule`).
Сборка — `tsc -b` (`src/js/tsi/r.sh` для контрактов, `src/js/ts/r.sh` для тестов/скриптов).
