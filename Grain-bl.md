
# Grain: Operating System for Distributed Commerce (The Marketplace Killer)

*A technical specification combining high-performance contract execution with a practical marketplace example*

---

## 1. High-Speed Data Updates

- **Grains** — minimal state units (order line, product stock, user profile)
- Update **only changed leaves and their parents** → `O(log N)`
- No full state tree rebuilding

> Enables high transaction throughput without global recomputation

## 2. Contract Execution Speed

- JS contracts (QuickJS) in isolated environment
- **~20 µs per method call** on a single thread
- Hot contracts — cached (~15 µs), cold contracts — loaded (~70 µs)

> 1 thread → ~50k calls/second. Horizontal scaling via multithreading and sharding

## 3. Internal Contract Calls

- Contracts call each other → **service ecosystem**:
  - Marketplace calls logistics contract
  - Payment contract calls staking
  - Analytics subscribes to events
- Services remain **loosely coupled** and shareable

## 4. Read Query Sharding

- **SELECT** queries are sharded across nodes
- Adding nodes → increased read throughput
- Classic CQRS model: writes via consensus, reads via any node

## 5. Light Nodes on Client (Mobile Apps)

- Light node stores only relevant grains
- Example: marketplace with 10M products → full state **~10 GB**
- 128–256 GB phone handles it without issues

## 6. Offline Browsing

- Mobile app works **without network** using local grain copy
- Hash‑verified integrity
- Upon reconnection — sync changes

## 7. Direct HTTP Access to Grains

```
GET /grain/{type}/{id}?hash={expected_hash}
```
- Server — ordinary HTTP server (nginx, actix)
- Client verifies hash and can cache locally
- Enables CDN for state, faster than RPC, no block recomputation

## 8. Emit → Analytics (Relational DB)

- Contracts **emit events** on state changes
- Events are dumped to relational DB (PostgreSQL / ClickHouse)
- Used for complex JOIN queries, reporting, BI
- Network itself doesn't do JOINs — that's the DB's zone

## 9. Building the Relational Database

- Take **state at period start** (snapshot)
- Apply blocks incrementally (as events)
- Database is fully reproducible and deterministic

## 10. Trust in Relational DB Data

- DB admin cannot forge data without breaking consensus
- State verified via grain hashes
- Any mismatch → immediate detection
- **No trust in administrator** — only cryptographic guarantees

---

## 🛒 Example: Marketplace on Grain

### The Web 2.0 Problem

A centralized marketplace requires:
- Data center (e.g., 64 MW like Ozon) → **up to $100M/month** for power and hardware
- Thousands of developers, analysts, managers
- High entry barrier for sellers (commissions, fines, advertising)
- Data monopoly on sales, user behavior, logistics

> **Seller is not a participant — they're a customer of the system.**

### The Grain Solution

1. **Marketplace smart contract** provides seller interface:
   - Sellers register, their addresses go into contract ACL
   - Marketplace takes commission for contract method execution (e.g., **1%**)

2. **Each seller** can write their own contract that computes:
   - product price
   - delivery time
   - availability

   Marketplace queries these contracts in real time via internal calls and displays a product list with different prices and delivery options.

3. **1% commission** is distributed among:
   - seller
   - marketplace
   - node operators (state hosts)

   > Nobody works for free, but monopoly is impossible.

4. **Mobile app as light node**:
   - Host your products directly from your phone
   - Offline purchases and browsing
   - 10M products → ~10 GB on phone

   > Seller can work **from warehouse AND smartphone**

### Competition at Every Level

| Level | Who Competes |
|-------|---------------|
| Sellers | price, delivery time, contract quality |
| Marketplaces | anyone can launch their own aggregator contract |
| Nodes | state hosts earn commission |
| Apps | any client (iOS, Android, Web) |

> **No monopoly points.**

### Economics vs Web 2.0

| Web 2.0 Cost Item | Grain Solution |
|-------------------|----------------|
| Data center (64 MW) | No central data center |
| Product hosting | Client / light nodes |
| Logistics API | Seller contracts |
| Development team | Open source contracts |
| Marketplace commission | ~1% (flexible, competitive) |
| Entry barrier | Zero: write contract → register |

> **Winner** = whoever has the better contract, not whoever bought more ads or servers.

---

## 📊 Grain Feature Summary

| Feature | Value |
|---------|-------|
| Update speed | `O(log N)` |
| Contract call | ~20 µs (hot) / ~70 µs (cold) |
| Horizontal reads | sharding |
| Mobile light node | 10 GB for 10M products |
| Offline access | ✅ |
| HTTP to grains | by path + hash |
| Analytics | emit → relational DB |
| Analytics trust | cryptographic |
| Marketplace commission | 1% (example) |

---

## 🧠 Summary

**Grain is not "just another blockchain" — it's a pragmatic business platform:**

✅ High speed, low cost  
✅ Distributed economy, cheaper than Web 2.0 data centers  
✅ Zero entry barrier for sellers and marketplaces  
✅ Contract competition, not capital competition  
✅ A tool for commerce demonopolization  

**Grain = Operating System for Distributed Business**
