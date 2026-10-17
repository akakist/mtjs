registerMethod("peek", function(params) {
    return callView("transfer", "balanceOf", params);
});

registerMethod("callBad", function(params) {
    return callView("transfer", "badView", params);
});

registerMethod("peekBalance", function(params) {
    let r = callView("transfer", "balanceOf", params);
    return r.balance;
});
