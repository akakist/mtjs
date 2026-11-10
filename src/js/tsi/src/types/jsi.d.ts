// JSI API контрактов (jsi/api/contract_api.cpp, register_api).
// Рантайм: quickjs_i.c/quickjs_i.h (int64-only, float64 удалён).
// banned (grep в r.sh, lib: ["es5"] их содержит, tsc НЕ ловит):
//   Math, JSON, Date, RegExp, Promise, Map, Set, WeakMap, WeakSet, Proxy, Reflect,
//   BigInt, ArrayBuffer, DataView, parseFloat, isNaN, isFinite.
// Доступно в JSI_AddIntrinsicBaseObjects: Object, Function, Error, Array, Number,
//   Boolean, String, Symbol, Generator, GeneratorFunction, parseInt, globalThis.
// int64: / — trunc (10/3=3), + - * — overflow → RangeError.
//
// ВАЖНО: view не получает msg (анонимный вызов, нет sender/value/nonce/времени).
//   Время (blockTimestamp) — только в method, через msg.

// ============================================================
// JsiGranule — данные гранулы (старый API: getGranule/createGranule)
// ============================================================
type JsiGranule = Record<string, any>;

// ============================================================
// JsiStorage — иерархия гранул (новый API: create_granule/get_granule)
// Аналог Cellable: parent (сырой), name, dbid, container (данные), ttl, update_time.
// flush дерева в состояние — после выполнения контракта (автоматически).
// ============================================================

interface JsiStorage {
    get(): any;                                      // value (JSON, null если пусто)
    get(name: string): JsiStorage | null;            // child (null если нет)
    get(name: string, ttl: number): JsiStorage;      // child orCreate
    set(value: any): void;
    del(): void;
}

interface JsiStorageView {
    get(): any;
    get(name: string): JsiStorageView | null;
    exists(): boolean;
}

interface JsiStorageWritable extends JsiStorageView {
    get(name: string, ttl: number): JsiStorageWritable;
    set(value: any): void;
    del(): void;
}

// interface JsiStorage {
//     // создать дочерний (ошибка, если уже есть)
//     create_granule(name: string, ttl: number): JsiStorage;

//     // получить дочерний (ошибка, если нет — try/catch в контракте)
//     get_granule(name: string): JsiStorage;

//     // данные (container = serialize(value))
//     set(value: any): void;
//     get(): any;   // null, если пусто

//     // удалить (bc_granule + bc_activity)
//     del(): void;
// }

    // interface JsiStorageView {
    //     get_granule(name: string): JsiStorageView;   // read, throw если нет
    //     exists(): boolean;
    //     get(): any;
    // }

    // interface JsiStorageWritable extends JsiStorageView {
    //     get_granule(name: string, ttl: number): JsiStorageWritable;   // orCreate
    //     set(value: any): void;
    //     del(): void;
    // }

// ============================================================
// JsiMsg — сообщение (для method)
// ============================================================
interface JsiMsg {
    readonly sender: string;
    readonly value: number;
    readonly gasLimit: number;
    readonly gasUsed: number;
    readonly blockTimestamp: number;
    readonly blockHeight: number;
    readonly nonce: number;
    readonly chainId: number;
    readonly txHash: string;
    readonly txIndex: number;
    readonly caller: string;   // CONTRACT_id.container ("" для внешнего)
}

// ============================================================
// Регистрация методов/views
// ============================================================
declare function registerMethod(name: string, fn: (params: any, msg: JsiMsg) => any): void;

declare function registerView(name: string, fn: (params: any) => any): void;

// ============================================================
// Старый API (плоские ключи): getGranule/createGranule/setGranule/deleteGranule
// ============================================================
declare function getGranule(key: string): JsiGranule | undefined;

declare function createGranule(key: string, value: any, ttl: number): void;

declare function setGranule(key: string, value: any): void;

declare function deleteGranule(key: string): void;

// ============================================================
// Новый API (иерархия): create_granule/get_granule
// ============================================================
// declare function create_granule(path: string, ttl: number): JsiStorage;

// получить корневой (ошибка, если нет)
// declare function get_granule(path: string): JsiStorage;
// ============================================================
// Прочее
// ============================================================
declare function emit(name: string, value: any): void;   // <-- any (не JsiStorage)

declare function callMethod(contract: string, method: string, params: object): any;

declare function callView(contract: string, view: string, params: object, timeout_sec: number): any;

declare function consumeValue(amount: number): void;

declare function refundValue(amount: number): void;

declare function remainingValue(): number;

// declare function get_granule(path: string): JsiStorageView;
// declare function get_granule(path: string, ttl: number): JsiStorageWritable;
declare function exists_granule(path: string): boolean;

declare function get(path: string): JsiStorage | null;
declare function get(path: string, ttl: number): JsiStorage;

// legacy alias
declare function get_granule(path: string): JsiStorage | null;
declare function get_granule(path: string, ttl: number): JsiStorage;

