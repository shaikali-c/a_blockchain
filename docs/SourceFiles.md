# Source File Reference

This document explains every `.h`, `.hpp`, `.cpp`, and test file in the project.

## Build Metadata

### `CMakeLists.txt`

Defines the `axis` project, requires C++23, discovers external dependencies, builds `axis_core`, `axisd`, and optional `axis_core_tests`. It also enables precompiled headers through `include/axis/pch.hpp`.

## Headers

### `include/axis/types.h`

Purpose: define fixed byte types, validation error enums, binary serialization helpers, and timestamps.

Defined symbols:

- aliases: `Hash`, `Address`, `PublicKey`, `SecretKey`, `Signature`
- enums: `TxError`, `BlockError`
- structs/classes: `Writer`, `Reader`, `Timestamp`

Why it exists: all higher-level modules need consistent binary representations and primitive read/write helpers.

### `include/axis/util.h`

Purpose: lightweight utilities for logging, hex conversion, and display formatting.

Defined symbols:

- namespace `logging`: `info`, `err`, `reject`
- templates: `to_hex`, `from_hex`
- helpers: `short_hex`, `short_addr`, `format_amount`, `format_timestamp`

Why it exists: keeps non-consensus display and conversion helpers out of core classes.

### `include/axis/crypto.h`

Purpose: declare the cryptographic API used by transactions, blocks, and validation.

Defined symbols:

- `blake2b`
- `compute_merkle_root`
- `derive_address`
- `verify_sig`
- `sign_msg`

### `include/axis/tx.h`

Purpose: declare transaction model types.

Defined symbols:

- `OutPoint`
- `std::hash<OutPoint>` specialization
- `TxOutput`
- `Transaction`
- `SignedTransaction`
- stream operators

### `include/axis/block.h`

Purpose: declare block header and block types.

Defined symbols:

- `BlockHeader`
- `Block`
- stream operators

### `include/axis/chain.h`

Purpose: declare the central chain state owner.

Defined symbols:

- `Chain`
- nested `Chain::HashHasher`
- forward declaration `leveldb::DB`

Important note: `pool_` is public in the current header even though it represents internal mempool state.

### `include/axis/net.h`

Purpose: declare the binary TCP server and message IDs.

Defined symbols:

- `MsgType`
- `ServerEvents`
- `Server`

### `include/axis/web.h`

Purpose: declare the Crow HTTP/WebSocket server.

Defined symbols:

- `WebServer`

### `include/axis/pch.hpp`

Purpose: precompiled header for stable third-party and standard library includes. It intentionally excludes project headers.

## Implementation Files

### `src/crypto.cpp`

Implements libsodium-backed hashing, Merkle root construction, address derivation, signature verification, and signing. Contains internal helper `combine_hash()`.

Dependencies: `axis/crypto.h`, libsodium, STL arrays/vectors.

### `src/tx.cpp`

Implements transaction txid computation, serialization/deserialization, and pretty-printing. Uses `blake2b()` for txids and `util.h` formatting for display.

Key behavior: deserialization trusts stored txid rather than recomputing it.

### `src/block.cpp`

Implements block header hashing/serialization, block construction/deserialization, block Merkle root calculation, and display output.

Key behavior: block construction computes Merkle root from transaction txids; deserialization recomputes cached block hash but does not verify Merkle root.

### `src/chain.cpp`

Implements the blockchain state machine, persistence, genesis creation, UTXO update logic, transaction validation, mempool handling, proof-of-work target construction, and block insertion.

Key behavior: `add_tx()` validates transactions; `add_block()` mutates chain state but assumes block acceptance checks already happened.

### `src/net.cpp`

Implements Asio TCP server, packet framing, request dispatch, binary payload parsers, response serializers, and TCP transaction/block submission paths.

Key behavior: block submission validation lives here rather than entirely inside `Chain`.

### `src/web.cpp`

Implements Crow routes, manual JSON serialization, HTTP transaction submission parsing, CORS responses, WebSocket connection tracking, and event broadcasts.

Key behavior: HTTP `rawTx` decodes the same payload layout as TCP `CreateTransaction`, not `Transaction::serialize()`.

### `src/main.cpp`

Process entry point. Initializes libsodium, constructs `Chain`, `WebServer`, and `Server`, wires callbacks, runs web server on a secondary thread and TCP server on the main thread.

## Tests

### `tests/core_serialization_tests.cpp`

A simple assert-based test executable covering:

- transaction serialization round-trip,
- block serialization round-trip,
- malformed empty transaction rejection by deserialization,
- coinbase detection,
- Merkle root computation for two leaves.

It calls `sodium_init()` before using crypto helpers.
