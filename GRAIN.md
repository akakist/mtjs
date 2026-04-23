# Grain

**Grain is a blockchain.  
More precisely — a deterministic [state machine](ca://s?q=Grain_state_machine) where the entire system state is stored as a granular Merkle tree.**

Each block represents a *state transition*.  
The Merkle root of the state tree is the canonical identifier of the network state and the subject of consensus.

---

## 🌱 Concept

In Grain, the entire state is represented as a single **[data tree](ca://s?q=Grain_data_tree)**.  
Each node in the tree contains:

- either raw data (leaf),
- or hashes of its children (internal node).

The root node contains the **[Merkle root](ca://s?q=Grain_merkle_root)** — a hash of the entire state.  
This root hash uniquely identifies the state after executing a block.

---

## 🌳 State Mechanics

### **Accessing data**
All reads and writes operate on **leaf nodes**.  
Each leaf has a deterministic path from the root — this path is the key in the underlying storage (RocksDB).

Whenever a leaf is accessed, it is marked as **dirty**.

### **Updating the state**
After executing a block:

1. Hashes of all dirty leaves are recalculated.  
2. If a leaf hash changes, its parent hash is recalculated.  
3. The process continues upward until the root.

Only the modified part of the tree is updated.  
This ensures deterministic execution and efficient state transitions.

---

## 🧩 Consensus

Nodes execute the same block of transactions and compute the resulting Merkle root.

If **70% of the stake** produces the **same root hash**,  
that state is considered valid and finalized.

> In Grain, the subject of consensus is not the block itself,  
> but the **post‑block state**.

---

## 🌾 Why “Grain”

The global state consists of many small **grains** — atomic fragments of data.  
Each grain is stored as a key–value pair in RocksDB:

key -> value


Where `key` is the deterministic path from the root.

Hence the name: **Grain — a blockchain composed of state grains**.

---

## 🧱 Data Addressing

Each smart contract has a **base address**.  
All of its internal data is stored under this prefix.

This provides:

- isolation between contracts,
- deterministic layout,
- no key collisions.

---

## 🗂️ Large Structures: granular_map

For large collections, Grain uses **[granular_map](ca://s?q=Grain_granular_map)** — a tree structure with base‑62 branching.

Example:

- level 1: 62 nodes  
- level 2: 62 × 62  
- level 3: 62 × 62 × 62  
- …

This allows storing **hundreds of millions of entries** efficiently.

Advantages:

- no MVCC, no WAL, no VACUUM, no indexes,
- deterministic structure,
- minimal update cost,
- scalable for very large datasets.

Granular_map is ideal for large maps, sets, and contract‑level storage.

---

## 📦 Storage Backend

Grain uses RocksDB as the underlying key–value store.  
Each node in the state tree is stored as:

path -> value


Where `path` is the canonical address of the node.

---

## 📘 Project Status

Grain is under active development.  
Architecture and implementation details may evolve as the protocol matures.

---

## 📄 License

MIT License.
