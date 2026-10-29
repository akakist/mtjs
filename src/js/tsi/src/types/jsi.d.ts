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
}

type JsiGranule = Record<string, any>;

declare function registerMethod(name: string, fn: (params: any, msg: JsiMsg) => any): void;
declare function registerView(name: string, fn: (params: any) => any): void;

declare function getGranule(key: string): JsiGranule | undefined;
declare function createGranule(key: string, value: JsiGranule, ttl?: number): void;
declare function setGranule(key: string, value: JsiGranule): void;

declare function emit(name: string, value: JsiGranule): void;

declare function callMethod(contract: string, method: string, params: object): any;
declare function callView(contract: string, view: string, params: object): any;
