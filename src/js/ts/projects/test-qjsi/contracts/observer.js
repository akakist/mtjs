registerMethod("peek", function(params, msg) {
    return callView("transfer", "balanceOf", params);
});

registerMethod("callBad", function(params, msg) {
    return callView("transfer", "badView", params);
});

registerMethod("callBadEmit", function(params, msg) {
    return callView("transfer", "badEmitView", params);
});

registerMethod("peekBalance", function(params, msg) {
    let r = callView("transfer", "balanceOf", params);
    return r.balance;
});