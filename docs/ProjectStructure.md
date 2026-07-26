# Project Structure

Axis has a small source tree with one CMake project, one core library, one daemon executable, and one test executable.

```text
axis/
├── CMakeLists.txt
├── CMakeSettings.json
├── README.md
├── include/axis/
│   ├── block.h
│   ├── chain.h
│   ├── crypto.h
│   ├── net.h
│   ├── pch.hpp
│   ├── tx.h
│   ├── types.h
│   ├── util.h
│   └── web.h
├── src/
│   ├── block.cpp
│   ├── chain.cpp
│   ├── crypto.cpp
│   ├── main.cpp
│   ├── net.cpp
│   ├── tx.cpp
│   └── web.cpp
├── tests/
│   └── core_serialization_tests.cpp
├── docs/
│   └── technical documentation
├── blocks/
│   └── runtime LevelDB chain database
└── pool/
    └── runtime LevelDB mempool database
```

## File-by-File Reference

| File | Purpose | Main symbols | Depends on | Used by |
| --- | --- | --- | --- | --- |
| `CMakeLists.txt` | Defines build requirements, dependencies, `axis_core`, `axisd`, and `axis_core_tests`. | CMake targets | Threads, LevelDB, PkgConfig, Crow, libsodium, Asio | Build tooling |
| `include/axis/types.h` | Fundamental data aliases and primitive binary serialization helpers. | `Hash`, `Address`, `PublicKey`, `SecretKey`, `Signature`, `TxError`, `BlockError`, `Writer`, `Reader`, `Timestamp` | libsodium constants, STL | Almost every module |
| `include/axis/util.h` | Logging, hex conversion, short display formatting, amount/timestamp formatting. | `logging::*`, `to_hex`, `from_hex`, `short_hex`, `short_addr`, `format_amount`, `format_timestamp` | `types.h`, libsodium hex helpers | Chain, tx, block, net, web |
| `include/axis/crypto.h` | Crypto API declarations. | `blake2b`, `compute_merkle_root`, `derive_address`, `verify_sig`, `sign_msg` | `types.h` | Chain, tx, block, tests |
| `src/crypto.cpp` | Crypto implementation using libsodium. | Same as `crypto.h`; internal `combine_hash` | libsodium | Core library |
| `include/axis/tx.h` | Transaction model declarations. | `OutPoint`, `TxOutput`, `Transaction`, `SignedTransaction`, stream operators | `types.h` | Block, Chain, Server, WebServer |
| `src/tx.cpp` | Transaction hashing, serialization/deserialization, pretty printing. | `Transaction::compute_hash`, constructors, `serialize`, `deserialize`, `pretty`, stream operators | `crypto.h`, `util.h` | Core library, tests |
| `include/axis/block.h` | Block and block header declarations. | `BlockHeader`, `Block`, stream operators | `tx.h` | Chain, Server, WebServer |
| `src/block.cpp` | Block hashing, Merkle construction, serialization/deserialization, display. | `BlockHeader::*`, `Block::*`, internal `compute_block_merkle_root` | `crypto.h`, `util.h` | Core library, tests |
| `include/axis/chain.h` | Chain state and validation/persistence interface. | `Chain`, `HashHasher` | `block.h`, LevelDB forward declaration, mutex/STL | Server, WebServer, main |
| `src/chain.cpp` | Chain construction, LevelDB load/store, genesis, UTXO mutation, mempool validation, block insertion. | `Chain::*`, internal `hex_key`, static `GENESIS_ADDR` | LevelDB, crypto, util | Core library |
| `include/axis/net.h` | Binary TCP protocol declarations. | `MsgType`, `ServerEvents`, `Server` | `chain.h`, Asio | main |
| `src/net.cpp` | Asio accept/session loop and TCP packet handlers. | `Server::*`, `ParsedCreateTx`, `ParsedCreateBlock`, parsers and response helpers | block, tx, util, chain | Core library |
| `include/axis/web.h` | Crow HTTP/WebSocket API declarations. | `WebServer` | `chain.h`, Crow | main |
| `src/web.cpp` | HTTP routes, JSON response builders, WebSocket connection management and broadcasts. | `WebServer::*`, JSON helpers, payload parser | Crow, chain, util | Core library |
| `include/axis/pch.hpp` | Precompiled header for stable third-party and standard headers. | None | Asio, LevelDB, libsodium, STL | CMake PCH for targets |
| `src/main.cpp` | Process entry point and object wiring. | `main` | chain, net, util, web, libsodium | `axisd` |
| `tests/core_serialization_tests.cpp` | Minimal regression tests using `assert`. | `test_tx_roundtrip`, `test_block_roundtrip`, `test_malformed_rejected`, `test_coinbase`, `test_merkle`, `main` | block, crypto, tx, libsodium | `axis_core_tests` |

## Header/Implementation Pairing

Most modules follow a one-header/one-source pattern:

- `tx.h` ↔ `tx.cpp`
- `block.h` ↔ `block.cpp`
- `crypto.h` ↔ `crypto.cpp`
- `chain.h` ↔ `chain.cpp`
- `net.h` ↔ `net.cpp`
- `web.h` ↔ `web.cpp`

`types.h` and `util.h` are header-only. `pch.hpp` is included by CMake as a precompiled header rather than by source files directly.

## Runtime Directories

`Chain` opens two LevelDB databases relative to the process working directory:

| Directory | Opened by | Contains | Created if missing |
| --- | --- | --- | --- |
| `blocks/` | `Chain::Chain()` | Serialized `Block` values keyed by zero-padded block height. | Yes |
| `pool/` | `Chain::Chain()` | Serialized pending `Transaction` values keyed by transaction hash hex. | Yes |

These directories are excluded from codebase indexing because they are runtime data, not source.

## Navigation Tips

- Start with `src/main.cpp` for startup and object lifetime.
- Read `include/axis/types.h` before any serialization/protocol work.
- Read `src/chain.cpp` for state mutation and validation rules.
- Read `src/net.cpp` for binary protocol behavior and block submission rules.
- Read `src/web.cpp` for HTTP/WebSocket routes and JSON shapes.
