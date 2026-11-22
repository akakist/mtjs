# AGENTS.md — MTJS (GRAIN) node

Рабочий снимок проекта для агента. Проект: C++/QuickJS блокчейн-нода.
Удалённая сборка: `<build-host>:~/mtjs` (`ssh <build-host> 'cd ~/mtjs && ...'`).

## Сборка и тесты
- C++: `cd ~/mtjs/src/build && make -j16`
- TS контракты: `cd ~/mtjs/src/js/tsi && ./r.sh` (tsc -b)
- TS тесты: `cd ~/mtjs/src/js/ts && ./r.sh` (tsc -b)
- Полный регресс: `cd /tmp/regress && ./b_run-all.sh` (26 тестов)
- Кластер вручную: `cd /tmp/regress && ./restart2.sh` (10 нод, свежая БД), лог `cluster.log`
- Правки — только через scp/ssh на билд-хост; локальной сборки нет.
- Каждый тест с гранулами гонять на свежем кластере (иначе «x exists» из прошлого прогона).

## Ключевые конвенции
- `REF_getter<T>` — владение; `new ...`; `Refcountable(const char* nm)` без default-ctor.
- `TIME_mks` (мкс, поле `.v`), `TTL_mks`, `to_mks()`, `TIME_sec`; `iUtils->getNow()` = wall-clock µs.
- `epoch_current()` = `prev_block->new_epoch + 1` (детерминированно; у первого блока 0).
- Паттерн `getXNoCreateConst(...)` — const-доступ без создания (NodeMsgSource).
- `MUTEX_INSPECTOR`, `JSI_GetContextOpaque`/`JSI_SetContextOpaque` (контекст вызова).
- Снапшот/грязность: аксессор делает markDirty; `update_time` только монотонно (`t > _update_time`).

## Типы целых и форматы
- **Натив (нода)**: value/consume_value/refund/fees/gas — `int64`; балансы/стейки/supply — `i128`.
- **Контракт (JSI)**: все целые — QuickJS `BigInt`.
- Граница `int64/i128 <-> BigInt` — только на входе/выходе JSI.
- `i128` в ioBuffer: zigzag + LEB128 (`put_i128`/`get_i128`), отдельно от int64 `put_PN`/`get_PN`.
- Гранула/эмит — один бинарный формат `[fmt: 0=CPY|1=ZSTD|2=LZ4][pack_jsi]`; ключи объекта отсортированы.
- Чтение гранул — только бинарь: `granule_unpack`/`unpack_jsi`; JSON-строки не принимаются.

## Свежие архитектурные решения
1. **CallContext** (`9d735db`): `ExecContext : Refcountable` (const `Mode` + const `now`)
   → `TxContext`/`TxViewContext`/`QueryContext`. tx/view: `now`=block_timestamp (чейн);
   query: `now`=wall-clock + отдельный `block_timestamp` (чейн, для `query.blockTimestamp`).
   Tx-only поля (roll/emits/value_ptr/fees/deleted_granules) только в `TxContext`.
2. **Живость** (`8c45a47`): `last_update` (TIME_mks) по подписанному alive-сету (пинг BLS).
   - PingRSP подписан BLS; агрегат `agg_alive_sig` + `alive_validators` в `BlockAccepted2REQ`.
   - `mark_activity` (execute_block): для alive-сета → `activity._update_time` (GC) + `node.last_update` (кэш).
   - vne = `enabled && node_active`; `node_active`: `last_update==0` → true (sentinel «ещё не пинговал»),
     иначе `block_ts - last_update <= 60с`.
   - Лидер: `current_leader()` = `committee[slot % size]`; блок доделывается с замороженным `cli.node_leader`.
3. **Хранение** (`cdbe710`): контракт НЕ удаляет гранулы (deleteGranule убран).
   - Правило: «выставил TTL, нет активности → удалилось само».
   - Внутреннее удаление (GC по TTL, компакция, apply-diff) остаётся.
4. **BigInt в qjsi**: восстановлен из оригинального QuickJS (ядро `jsi_bigint_*`, конструктор,
   парсер литерала `Nn`, `JSI_AddIntrinsicBigInt`, класс `JSI_CLASS_BIG_INT`); `Number(bigint)` → int64.
5. **Пулы tx**: `AddTxREQ` кладёт tx в локальный пул и пушит лидеру по IP через `ForwardTxREQ`
   (`GetTxREQ` кэширует route к лидеру). Раньше tx мог не дойти (нода early-return в GetTxREQ).

## Длительный прогон (soak)
- `/tmp/regress/soak.sh` — стресс на сутки+: нагрузка (тесты по кругу) + мягкие
  рестарты (краш-восстановление) + контроль стола (blocks) и высот (chk_heights.js).
  Запуск: `cd /tmp/regress && nohup ./soak.sh &`; следить: `tail -f /tmp/regress/soak.log`.
- Высоты читаются `mtjs.get_state().block_height` (= new_epoch); при блок-продакшене
  нормальный разброс ±1 блок.

## Дальше (бэклог)
- clock-lag (`MTJS_CLOCK_LAG_US`) реализован; мульти-процесс soak (`restart_mp.sh`) — не заведён.
- Сортировка дерева рассылки по `last_update` — отложена (валила регресс).
- rpcService-сокеты (stale Session) — исторически отложено.
- `chk-state1`: `tx_count` состояния не растёт (ячейка состояния читается детачнутой).

## Важное
- Тестовые leveldb (`src/js/ts/dist/db*`, `src/js/ts/dist/storage/db*`) — в `.gitignore`, не коммитить.
- Мусор (`rs.sh`, `apply_edit`, `*.tar`, `*.patch`, `dist/z`, `*.tsbuildinfo`) — не коммитить.
