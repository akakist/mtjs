var TTL_EVENT = 86400 * 30;
var TTL_TICKET = 86400 * 30;

registerMethod("createEvent", function(p: { name: string, sectors: number, rows: number, seats: number, price: number }, m: JsiMsg) {
    var event = get("event/" + String(p.name), TTL_EVENT);
    if (event.get()) throw new Error("event exists");
    event.set({ name: p.name, price: p.price, total: p.sectors * p.rows * p.seats, sold: 0 });

    for (var si = 0; si < p.sectors; si++) {
        var sector = event.get("Sector" + si, TTL_EVENT);
        sector.set({ rows: p.rows, seats: p.seats });
        for (var ri = 0; ri < p.rows; ri++) {
            var row = sector.get("row" + ri, TTL_EVENT);
            row.set({ seats: p.seats });
        }
    }
    emit("EventCreated", { name: p.name, total: p.sectors * p.rows * p.seats });
});

registerMethod("buyTicket", function(p: { event: string, sector: number, row: number, seat: number, amount: number }, m: JsiMsg) {
    var event = get("event/" + String(p.event), TTL_EVENT);
    var e = event.get();
    if (!e) throw new Error("event not found");
    var sector = event.get("Sector" + p.sector, TTL_EVENT);
    var row = sector.get("row" + p.row, TTL_EVENT);
    var seat = row.get("seat" + p.seat, TTL_TICKET);
    var s = seat.get();
    if (s && s.sold) throw new Error("already sold");

    consumeValue(p.amount);
    seat.set({ owner: String(m.sender), paid: p.amount, sold: true });

    var userTickets = get("user_tickets", TTL_EVENT);
    var ut = userTickets.get(String(m.sender), TTL_EVENT);
    var t = ut.get() || { tickets: [] };
    t.tickets.push("event/" + p.event + "/Sector" + p.sector + "/row" + p.row + "/seat" + p.seat);
    ut.set(t);

    e.sold = Number(e.sold) + 1;
    event.set(e);
    emit("TicketSold", { event: p.event, sector: p.sector, row: p.row, seat: p.seat, owner: String(m.sender), paid: p.amount });
});

registerView("getTicket", function(p: { event: string, sector: number, row: number, seat: number }) {
    var event = get("event/" + String(p.event));
    if (!event) return { found: false };
    var sector = event.get("Sector" + p.sector);
    if (!sector) return { found: false };
    var row = sector.get("row" + p.row);
    if (!row) return { found: false };
    var seat = row.get("seat" + p.seat);
    if (!seat) return { found: false };
    var s = seat.get();
    if (!s) return { found: false };
    return { found: true, owner: s.owner, paid: s.paid, sold: s.sold };
});

registerView("getEvent", function(p: { event: string }) {
    var event = get("event/" + String(p.event));
    if (!event) return { found: false };
    var e = event.get();
    return e ? { found: true, name: e.name, price: e.price, total: e.total, sold: e.sold } : { found: false };
});


registerMethod("deleteEvent", function(p: { name: string }, m: JsiMsg) {
    var event = get("event/" + String(p.name));
    if (!event) return;
    // удалить всё дерево (все секторы/ряды/места)
    var e = event.get();
    if (e && e.sectors) {
        for (var si = 0; si < e.sectors; si++) {
            var sector = event.get("Sector" + si);
            if (!sector) continue;
            var sec = sector.get();
            if (sec && sec.rows) {
                for (var ri = 0; ri < sec.rows; ri++) {
                    var row = sector.get("row" + ri);
                    if (!row) continue;
                    var r = row.get();
                    if (r && r.seats) {
                        for (var ti = 0; ti < r.seats; ti++) {
                            var seat = row.get("seat" + ti);
                            if (seat) seat.del();
                        }
                    }
                    row.del();
                }
            }
            sector.del();
        }
    }
    event.del();
    emit("EventDeleted", { name: p.name });
});
