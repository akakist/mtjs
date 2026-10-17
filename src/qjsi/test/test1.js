#!../out/build/qjsi/test/test_runner
registerMethod("transfer", function(params) {
    let from = getGranule(params.from);
    let to = getGranule(params.to);
    
    if (from.balance < params.amount) {
        throw new Error("insufficient balance");
    }
    
    from.balance -= params.amount;
    to.balance += params.amount;
    
    setGranule(params.from, from);
    setGranule(params.to, to);
});

registerView("balanceOf", function(params) {
    return getGranule(params.address).balance;
});