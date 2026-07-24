# Design decisions

This document records the rationale behind significant design decisions in
the Axis codebase. Each entry explains the problem, the chosen solution,
the alternatives considered, and the reasoning.

---

## 1. No `Blockchain` god class

**Decision:** Split into `Chain` (business logic), `Block` (data model),
`Transaction` (data model), `Server` (network), `Writer`/`Reader`
(serialization).

**Alternatives:**
- One `Blockchain` class handling everything (the original design)

**Reasoning:**
A 500-line class doing chain logic, networking, validation, storage, and
mining is hard to understand, test, and modify. By splitting into focused
classes with clear responsibilities, each piece can be understood in
isolation. `Chain` is still the largest class (~300 lines) but it only
does one thing: manage the node state.

---

## 2. No database wrapper

**Decision:** Use LevelDB directly with `unique_ptr` for RAII, not a custom
`DatabaseManager` class.

**Alternatives:**
- A `DatabaseManager` wrapper with `put_block`, `get_block`, etc.
- An ORM-like abstraction

**Reasoning:**
The wrapper adds no value — it was three passthrough methods. LevelDB's API
is already simple (`Put`, `Get`, `Delete`, `Iterator`). Wrapping it adds
lines of code without any capability. Direct usage makes it obvious what
database operations are happening.

---

## 3. No `common.h` junk drawer

**Decision:** Every type lives in a logically named header.

**Alternatives:**
- One `common.h` with all includes, type aliases, and utility functions

**Reasoning:**
A `common.h` creates unnecessary recompilation dependencies (changing a
small utility force-recompiles everything) and hides the module structure.
Each header now includes only what it needs.

---

## 4. UTXO keys as binary `OutPoint` (not hex strings)

**Decision:** `unordered_map<OutPoint, TxOutput>` instead of
`unordered_map<string, TxOutput>`.

**Alternatives:**
- Hex string keys: `"abc123...def789:0"` (original design)
- `map` with custom comparator

**Reasoning:**
The original design converted OutPoints to hex strings for every lookup.
This allocated strings on each UTXO operation (spending, querying). A
hex string for a 32-byte hash + index is ~66 bytes per operation. Using
binary OutPoint keys eliminates all string allocation from UTXO operations.

---

## 5. Explicit typed serialization (not `put(T)` template)

**Decision:** `Writer::put_u32`, `put_hash`, `put_addr`, etc. instead of
`Writer::put(T val)`.

**Alternatives:**
- Template `put<T>(T val)` that uses `sizeof(T)` and `memcpy`
- A serialization framework like Boost.Serialization or Cap'n Proto

**Reasoning:**
The original template `put(T)` worked on most types but silently truncated
`std::array` types on GCC (because `sizeof(std::array<uint8_t, 32>)` is 32,
but the template deduced `T` as... actually the issue was that `std::array`
was passed by value and only 8 bytes were written on some platforms due to
ABI quirks). Explicit methods guarantee the correct number of bytes are
written for every type. They also make the wire format obvious from the
serialization code.

---

## 6. C++20 coroutines (not callback-style Asio)

**Decision:** Use `co_await`/`co_spawn` for network I/O.

**Alternatives:**
- Traditional Asio with completion handlers (callbacks)
- Thread-per-connection
- A framework like libuv or libevent

**Reasoning:**
Coroutines make asynchronous code read as sequential code. Each session
looks like "read header → read payload → dispatch → send response" without
callback nesting. Thread-per-connection would waste memory (stack per
connection). Callbacks create "callback hell" as complexity grows.
C++20 coroutines are supported by GCC 14+ and Asio provides built-in
awaitable support.

---

## 7. No virtual dispatch, no inheritance, no templates (except STL)

**Decision:** Plain structs and classes. No base classes, no virtual
methods, no class templates.

**Alternatives:**
- Abstract base classes for storage backends
- Template-based serialization
- Policy-based design

**Reasoning:**
Axis has exactly one storage backend (LevelDB), one serialization format
(binary), and one crypto library (libsodium). There is no scenario where
you'd swap these at runtime or compile time. Virtual dispatch adds vtable
overhead and makes optimization harder. Templates increase compile time
and error message complexity. Plain code is simpler and faster.

---

## 8. No pretty printer

**Decision:** Removed the 170-line `PrettyPrinter` class that drew ASCII
trees.

**Alternatives:**
- Keep it as a debug-only utility

**Reasoning:**
170 lines for a feature that is never called from production code is dead
weight. The tree images it drew were not useful for debugging (you need
a debugger, not ASCII art). If visual inspection is needed, format the
data with standard tools (e.g., `hash_to_hex` + `printf`).

---

## 9. Static difficulty (no retargeting)

**Decision:** Difficulty is a hardcoded value of 3.

**Alternatives:**
- Bitcoin-style retargeting every N blocks
- Dynamic difficulty based on hash rate

**Reasoning:**
Axis is a demonstration/educational blockchain with no mining hardware
participating. Difficulty retargeting adds complexity (storing per-block
timestamps, computing averages) with no benefit. If real mining is added,
retargeting is the obvious change.

---

## 10. LevelDB (not SQLite or custom file format)

**Decision:** LevelDB as the storage engine.

**Alternatives:**
- SQLite (full SQL database)
- Custom file format (raw files per block)
- No persistence (in-memory only)

**Reasoning:**
LevelDB is fast (no SQL parsing, no query planning), embedded (no server),
and supports ordered iteration (critical for block height enumeration).
SQLite would be more flexible for ad-hoc queries but adds a dependency
with more surface area. A custom file format would require implementing
crash recovery, compaction, and key lookup — all of which LevelDB provides.

---

## 11. No UTXO index by address

**Decision:** `get_utxos` scans the entire UTXO set.

**Alternatives:**
- Maintain a `multimap<Address, OutPoint>` as a secondary index

**Reasoning:**
With a single genesis UTXO, there is nothing to index. Adding a secondary
index doubles the bookkeeping (insert on apply, delete on spend) and memory
usage. When the UTXO set grows (millions of entries), a secondary index
becomes necessary. The current design makes this easy to add: maintain a
parallel `unordered_multimap<Address, OutPoint>` updated in `apply_tx`.

---

## 12. Single I/O thread

**Decision:** One `asio::io_context` running on one thread.

**Alternatives:**
- Thread pool for the io_context
- Separate thread for validation

**Reasoning:**
Currently, CPU work (validation, serialization) is done on the I/O thread,
which blocks all other connections during processing. This is fine for the
current scale (single user, small chain). For production, validation should
be offloaded to a thread pool, or `asio::io_context` should use multiple
threads.

---

## 13. No config file or command-line argument parsing

**Decision:** Hardcoded port (8080) and data directory (`./axis_data/`).

**Alternatives:**
- argparse or getopt
- TOML/YAML config file
- Environment variables

**Reasoning:**
Adding argument parsing would add ~30 lines for the parser plus the
plumbing to pass values through constructors. With exactly one argument
(`--help`), this is not justified. When more configurability is needed (port,
data dir, network parameters), add a simple option parser.

---

## 14. No peer-to-peer networking

**Decision:** Axis runs as a standalone node accepting wallet connections.

**Alternatives:**
- Full P2P network with peer discovery, handshake, and block relay
- Bitcoin wire protocol compatibility

**Reasoning:**
A P2P layer requires peer discovery, connection management, inventory relay,
block relay, and transaction relay — easily doubling the codebase. The
current architecture makes Axis testable and useful for development. P2P
can be added as a separate module (`src/p2p.cpp`) that depends on `Chain`.
