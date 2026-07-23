# Design Decisions

This document explains important design decisions in Axis, why they were likely made, what alternatives exist, and what tradeoffs they create.

## 1. UTXO model instead of account-balance model

### Decision

Axis tracks value as unspent outputs referenced by inputs.

### Why it was chosen

The UTXO model is a classic blockchain design and is excellent for teaching transaction validation fundamentals.

### Advantages

- makes ownership validation explicit,
- naturally supports multiple inputs and change outputs,
- makes double-spend logic easier to reason about.

### Disadvantages

- more complex than a single balance number,
- requires UTXO lookup logic and indexing.

### Alternative

An account-based model storing balances directly per address.

### Tradeoff

UTXO teaches richer blockchain mechanics at the cost of more bookkeeping.

## 2. Singleton `Blockchain`

### Decision

Use `Blockchain::getInstance()` for the main service object.

### Why it was chosen

Simple startup and one obvious source of truth.

### Advantages

- easy access,
- straightforward lifetime,
- low boilerplate.

### Disadvantages

- harder to test in isolation,
- high coupling,
- encourages the class to accumulate responsibilities.

### Alternative

Dependency-injected service composition.

### Tradeoff

Less infrastructure complexity now, more architectural pressure later.

## 3. One central orchestration class

### Decision

Let `Blockchain` manage state, validation, persistence coordination, and networking.

### Why it was chosen

Educational readability and fewer files.

### Advantages

- easy to trace end-to-end behavior,
- low conceptual overhead for a small codebase.

### Disadvantages

- violates single-responsibility principles,
- harder to extend cleanly.

### Alternative

Separate `Mempool`, `ChainState`, `ProtocolHandler`, and `NodeServer` classes.

## 4. LevelDB for persistence

### Decision

Use LevelDB key/value stores for blocks and mempool data.

### Why it was chosen

Embedded, lightweight, simple API, good fit for serialized binary blobs.

### Advantages

- no external DB server,
- ordered iteration useful for blocks,
- small wrapper code.

### Disadvantages

- no relational querying,
- no rich schema management,
- no multi-record transaction semantics at application level by default.

### Alternative

SQLite, RocksDB, or custom flat files.

## 5. Store blocks as serialized blobs keyed by padded height

### Decision

Persist each block under a zero-padded numeric string key.

### Why it was chosen

This preserves natural ordering during iteration.

### Advantages

- simple recovery logic,
- no separate height index required.

### Disadvantages

- block lookup by hash still needs in-memory indexing,
- chain reorganization support would be more complex later.

## 6. Rebuild UTXO state by replaying blocks

### Decision

Do not persist a dedicated UTXO database in the current implementation.

### Why it was chosen

Simplifies consistency: the chain database is treated as canonical history.

### Advantages

- simpler persistence model,
- fewer moving parts,
- easier conceptual correctness.

### Disadvantages

- slower startup as history grows,
- no fast recovery shortcut.

### Alternative

Persist a materialized UTXO index and checkpoint it.

## 7. Persist the mempool

### Decision

Store pending transactions in `pool/` and reload them on startup.

### Why it was chosen

Prevents losing user-submitted pending transactions after restart.

### Advantages

- more durable node behavior,
- preserves pending-work context.

### Disadvantages

- requires re-reserving inputs on startup,
- needs cleanup once block inclusion exists.

## 8. Manual binary serialization

### Decision

Use custom byte layouts with `BytesWriter`, `BytesReader`, and `memcpy`.

### Why it was chosen

Simple, compact, and educational.

### Advantages

- small code,
- efficient encoding,
- easy to inspect field ordering in source.

### Disadvantages

- poor long-term compatibility story,
- native-layout dependence,
- more risk of subtle parsing bugs.

### Alternatives

- protobuf,
- flatbuffers,
- CBOR,
- explicit endian-aware custom codec.

## 9. Native integer layout and `size_t` in formats

### Decision

Write integers as their in-memory bytes, including `size_t` in block serialization.

### Why it may have been chosen

Convenience and minimal code.

### Advantages

- easy implementation,
- no conversion helpers required.

### Disadvantages

- not architecture portable,
- risky for stable wire protocols or cross-platform persistence.

### Recommended future alternative

Fixed-width integer encoding with explicit endianness.

## 10. Use libsodium

### Decision

Depend on libsodium for hashing and signature verification.

### Why it was chosen

Well-known modern crypto library with safe APIs.

### Advantages

- avoids hand-rolled cryptography,
- provides Ed25519 and hashing primitives.

### Disadvantages

- external dependency,
- current code still needs wallet/key-generation logic around it.

## 11. Derive addresses from public-key hash

### Decision

Compute a 20-byte address by hashing the public key.

### Why it was chosen

Shorter identifier than the full public key.

### Advantages

- compact owner representation,
- easy sender/public-key consistency checks.

### Disadvantages

- custom address format with no checksum or human encoding,
- not directly user-friendly.

## 12. Reserve mempool inputs separately

### Decision

Maintain `mempoolInputs` to track inputs already used by pending transactions.

### Why it was chosen

The UTXO set alone cannot stop two unconfirmed transactions from spending the same output.

### Advantages

- simple pending double-spend prevention,
- fast conflict detection.

### Disadvantages

- extra state to maintain,
- requires cleanup when transactions are confirmed or dropped.

## 13. Hardcoded genesis block

### Decision

Bake genesis values directly into code.

### Why it was chosen

Every blockchain needs a shared origin.

### Advantages

- deterministic startup,
- no separate genesis file required.

### Disadvantages

- inflexible,
- changing it breaks compatibility with existing data.

## 14. Async TCP with coroutines

### Decision

Use Asio async I/O and `co_await` instead of blocking sockets.

### Why it was chosen

Modern C++ async style with readable control flow.

### Advantages

- scalable structure,
- cleaner than nested callbacks.

### Disadvantages

- still embedded inside `Blockchain`,
- current implementation only processes one message per accepted connection.

## 15. Verify block transactions by mempool membership

### Decision

`verifyBlock()` assumes non-coinbase transactions are already validated if they exist in the mempool.

### Why it was chosen

Avoids redoing the full validation path.

### Advantages
n
- simpler logic,
- reuses mempool as a validation gate.

### Disadvantages

- assumes mempool is trustworthy and still in sync,
- insufficient for full independent block validation in a more realistic multi-peer environment.

## 16. Minimal logger

### Decision

Use plain stdout logging functions.

### Why it was chosen

Smallest possible logging layer.

### Advantages

- trivial to understand,
- no dependency or configuration burden.

### Disadvantages

- no timestamps,
- no structured fields,
- no filtering or persistence.

## 17. Summary

Axis makes many decisions that are very reasonable for an educational blockchain core:

- prefer clarity over abstraction,
- prefer explicit state over hidden machinery,
- prefer simple persistence over advanced indexing,
- prefer compact custom bytes over framework-heavy protocols.

Those choices make the code approachable, while also defining the exact places future maintainers will want to strengthen as the project grows.
