# Axis

Axis is an educational C++23 blockchain-node foundation. It maintains a small
proof-of-work chain, an unspent-transaction-output (UTXO) set, a transaction
mempool, LevelDB-backed persistence, and a compact binary TCP protocol.

It is a useful codebase for learning how blockchain data structures,
serialization, cryptographic signatures, and asynchronous networking fit
together. It is **not** production-ready cryptocurrency software.

## What is implemented

- UTXO-based transaction representation and validation.
- Ed25519 signature verification and public-key-derived addresses via
  libsodium.
- Transaction and block serialization/deserialization.
- Merkle-root calculation and basic block validation helpers.
- Persistent block and mempool databases using LevelDB.
- A standalone Asio TCP listener on port `9618`.
- Binary `GetUTXOs` and `CreateTransaction` messages.
- CTest regression checks for serialization round trips.

## Important scope limits

There is no wallet/key-management UI, miner, HTTP/REST server, peer discovery,
peer-to-peer synchronization, or public block-acceptance endpoint in the
current source tree. `verifyBlock()` exists as a helper but no implemented
network message commits a mined block. Treat this project as a learning and
extension base, not as a networked currency.

## Build and test

### Dependencies

- CMake 3.16 or newer
- A C++23 compiler
- standalone Asio headers
- libsodium 1.0.18 or newer
- LevelDB
- nlohmann_json and Crow (currently pulled in by shared project headers)

On an Arch-like system, the package names are typically `cmake`, `ninja`,
`libsodium`, `leveldb`, `asio`, `nlohmann-json`, and `crow`. Exact names vary
by platform.

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run the node from the repository root so its `blocks/` and `pool/` databases
are found in the expected relative locations:

```bash
./build/blockchain_tx
```

The process initializes libsodium, opens/creates the two LevelDB directories,
loads state, creates the genesis block if necessary, and then listens on TCP
port `9618`.

## Layout

```text
include/axis/       Public project headers, grouped by responsibility
src/blockchain/     Block, transaction, chain state, and TCP handlers
src/core/           Shared byte/hex/address helpers and logging
src/crypto/         Merkle-root implementation
src/storage/        LevelDB wrapper
tests/              Serialization regression tests
docs/               Beginner-friendly design and protocol documentation
blocks/              Runtime LevelDB chain data (created/updated at runtime)
pool/                Runtime LevelDB mempool data (created/updated at runtime)
```

## Start here

Read [Documentation index](docs/README.md), then
[Getting started](docs/getting_started.md) and
[Architecture](docs/architecture.md). The exact network bytes are specified in
[Packet protocol](docs/packet_protocol.md).
