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
    let g = getGranule(params.address);
    return { balance: g ? g.balance : 0 };
});

registerView("badView", function(params) {
    setGranule("x", { foo: 1 });
    return null;
});