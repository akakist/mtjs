registerMethod("parse", function(params, msg) {
    let radix = 0;   // 0 = автоопределение по префиксу
    if (params.radix !== undefined && params.radix !== null)
        radix = params.radix;
    return parseInt(params.s, radix);
});
registerMethod("parseAuto", function(params, msg) {
    return parseInt(params.s);
});
