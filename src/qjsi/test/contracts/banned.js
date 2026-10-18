// Date
registerMethod("badDate", function(p) {
    return new Date().getTime();
});
registerMethod("badDateNow", function(p) {
    return Date.now();
});

// async / Promise
registerMethod("badPromise", function(p) {
    return Promise.resolve(1);
});
registerMethod("badAsync", function(p) {
    // async function как выражение — проверит парсер
    let f = async function() { return 1; };
    return f();
});
registerMethod("badAwait", function(p) {
    // await вне async — синтаксис
    return (async function() { return await 1; })();
});

// Math
registerMethod("badMathRandom", function(p) {
    return Math.random();
});
registerMethod("badMathSqrt", function(p) {
    return Math.sqrt(4);
});
registerMethod("badMathMax", function(p) {
    return Math.max(1, 2);
});

// Proxy
registerMethod("badProxy", function(p) {
    return new Proxy({}, {});
});
registerMethod("badProxyRevocable", function(p) {
    return Proxy.revocable({}, {});
});

// Map / Set
registerMethod("badMap", function(p) {
    return new Map();
});
registerMethod("badSet", function(p) {
    return new Set();
});
registerMethod("badWeakMap", function(p) {
    return new WeakMap();
});
registerMethod("badWeakSet", function(p) {
    return new WeakSet();
});

// Reflect (если выпилен)
registerMethod("badReflect", function(p) {
    return Reflect.get({ a: 1 }, "a");
});

// BigInt
registerMethod("badBigInt", function(p) {
    return BigInt(1);
});
registerMethod("badBigIntLiteral", function(p) {
    return 1n;
});

// TypedArray / ArrayBuffer (если выпилены)
registerMethod("badInt8Array", function(p) {
    return new Int8Array([1, 2, 3]);
});
registerMethod("badArrayBuffer", function(p) {
    return new ArrayBuffer(8);
});
registerMethod("badDataView", function(p) {
    return new DataView(new ArrayBuffer(8));
});

// eval / Function
registerMethod("badEval", function(p) {
    return eval("1+1");
});
registerMethod("badFunction", function(p) {
    return new Function("return 1")();
});
registerMethod("badConstructorConstructor", function(p) {
    return ({}).constructor.constructor("return 1")();
});

// RegExp (рантайм)
registerMethod("badRegexLiteral", function(p) {
    return /abc/;
});
registerMethod("badRegexMatch", function(p) {
    return "abc".match(/b/);
});
registerMethod("badRegexReplace", function(p) {
    return "abc".replace(/b/, "x");
});
registerMethod("badRegexSplit", function(p) {
    return "abc".split(/b/);
});

// float
registerMethod("badFloatLiteral", function(p) {
    return 1.5;
});
registerMethod("badFloatExponent", function(p) {
    return 1e300;
});
registerMethod("badNaN", function(p) {
    return NaN;
});
registerMethod("badInfinity", function(p) {
    return Infinity;
});
registerMethod("badParseFloat", function(p) {
    return parseFloat("1.5");
});
