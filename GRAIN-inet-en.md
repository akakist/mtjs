
# Light Nodes & Granules: The Internet Without Servers, Monopolies, or Monthly Fees

## 1. The Problem with Today's Internet

Web 2.0 runs on servers. Servers need hosting, administration, security, backups, uptime.

The result:
- You pay **every month**, even when your site is idle.
- You pay **for protection** (Cloudflare), because otherwise bots will flood you.
- You depend on **a single hosting provider / a single company**.
- Your site can be **blocked** at any moment.

A small forum or blog costs **$30–500/month**.  
A medium marketplace costs **thousands of dollars**.  
A large one costs **a 64 MW data center and millions per month**.

This is **expensive, inconvenient, and unsafe**.

---

## 2. What Are Granules?

A granule is the smallest **self-contained piece of data** in the blockchain.

It can be:
- a single user's data
- one forum post
- one marketplace product
- the seller list for a single SKU
- an HTML page of a blog

**The key idea:**  
Every granule has its own hash. That hash is stored in its **parent granule**.  
The entire blockchain state is a tree of granules, folded into **a single root hash**.

> If you have the root hash, you can verify **any granule** by requesting only the granule itself and the merkle path to the root.

---

## 3. What Is a Light Node?

A light node is a client that **does not store the full state**.

It only stores:
- a trusted root hash (32 bytes)
- a cache of granules already requested (hash → data)

A light node **does not execute contracts**, does not participate in consensus.  
It only **requests granules on demand**.

---

## 4. How a Granule Request Works

1. The light node knows a path (e.g., `/users/alice/balance`).
2. It asks any full node (or CDN, or peer) for the **granule by path**.
3. The response includes:
   - the granule (data + hash)
   - a merkle proof (path from the granule hash to the root)
4. The light node verifies:
   - does the granule hash match?
   - does the proof lead to the trusted root hash?
5. If everything checks out, the granule is accepted and **cached locally**.

On repeat requests — instant response, no network needed.

---

## 5. Cost of Owning a Site: Web 2.0 vs Blockchain

### Web 2.0 (Today)
- Hosting: $10–200/month
- Cloudflare (bot protection): $20–200/month
- Backups, administration, monitoring: $50–1000/month
- Domain, SSL, email: $10–100/year

**Result:**  
Even a small forum or blog — **$30–500+ every month**.  
You pay **constantly**, even when your site is empty.

### Blockchain (Light Nodes + Granules)

- Deploy a contract (site, forum, blog): **$1–10 (one-time)**.
- Reading data: **free** (the light node caches granules).
- Post / comment / action: **$0.0001–0.001** (paid by users, not the owner).

**The owner pays nothing for hosting, nothing for protection, nothing for servers.**  
They pay **once**. That's it.

| | Web 2.0 (small forum) | Blockchain |
|---|---|---|
| Deployment | $10–50/month | **$1–10 one-time** |
| Bot protection | $20–200/month | **built into the economy** |
| Image hosting | separate fee | IPFS + hash in granule |
| Administration | $50+/month | **not needed** |
| Risk of blocking | high | **impossible** |

**The difference is orders of magnitude.**  
In Web 2.0, you pay every month to rent someone else's server.  
In the blockchain, you pay **only for actual usage** — and even that is paid by your users, not you.

---

## 6. What About Blocking? (Roskomnadzor and Others)

Roskomnadzor can block a specific IP or domain, but it cannot delete a granule — the granule has thousands of copies on thousands of nodes around the world, and only one needs to remain for the content to stay accessible.

In Web 2.0, a single request to a hosting provider is enough — the page disappears for everyone.  
In the blockchain, there is no hosting provider, no single server, no single domain.  
Content belongs to itself.

---

## 7. Why This Benefits Everyone

### For users:
- Reading is free.
- They only pay for actions (pennies).
- Their data does not go to a corporate server.

### For site / forum / marketplace owners:
- No hosting, no Cloudflare, no admin needed.
- The site cannot be blocked.
- Monetization through user-paid fees.

### For marketplace sellers:
- Commission 0.1–1%, not 20–55%.
- The same product can be sold by dozens of apps.
- No fear of being blocked.

### For investors / validators:
- Earn from transaction fees.
- No monopoly — the market sets the price.

---

## 8. What Changes in Practice

- Forums become lively again — spam is killed economically.
- Blogs don't need hosting — they live in granules.
- Marketplaces compete on interface, not on access to data.
- Search is back to 10 blue links — no ads, no algorithms.
- No one can delete your content.

---

## 9. Limitations (Honestly)

- Don't put photos or videos in granules. Store only the hash; the actual files go to IPFS / torrent.
- A network connection is required (offline = only cached content).
- A trusted root hash is needed (from genesis or a checkpoint).

These are **solvable limitations**, not fundamental problems.

---

## Conclusion

The internet on light nodes and granules —  
**this is not "crypto will replace the web".**

This is **the web finally becoming cheap, free, and resilient**.

- No recurring payments.
- No monopolies.
- No blocking.

You pay **once** — and your service lives as long as at least one node anywhere holds your granules.

The question is not *whether this will happen*.  
The question is — **who will do it first**.

