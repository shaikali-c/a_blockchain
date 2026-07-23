# Getting Started

This guide helps a new developer build, run, inspect, and mentally model Axis.

## 1. What you are looking at

Axis is a small C++23 blockchain node foundation. In simple terms, it is a server process that:

- stores blocks,
- stores pending transactions,
- keeps track of which coins are still spendable,
- accepts a few network messages over TCP,
- validates incoming transactions.

It is easiest to think of Axis as a **teaching implementation** of several blockchain building blocks rather than as a complete cryptocurrency.

## 2. Prerequisites

You need:

- CMake 3.16+
- a C++23 compiler
- libsodium
- LevelDB
- standalone Asio
- nlohmann_json
- Crow

Even though Crow and nlohmann_json are not central to the currently running node path, they are included by shared headers and therefore still participate in the build.

## 3. Build the project

From the repository root:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
```

## 4. Run tests

```bash
ctest --test-dir build --output-on-failure
```

The current test suite is small and mainly verifies serialization and deserialization round trips for `Transaction` and `Block`.

## 5. Run the node

```bash
./build/blockchain_tx
```

Run it from the repository root so the relative database paths `blocks/` and `pool/` resolve correctly.

## 6. What happens at startup

When the program starts, `axis/src/main.cpp` does the following:

1. Initializes libsodium with `sodium_init()`.
2. Retrieves the singleton `Blockchain` instance.
3. The `Blockchain` constructor opens LevelDB databases.
4. It loads previously stored blocks.
5. It loads previously stored mempool transactions.
6. If no blocks exist, it creates a hardcoded genesis block.
7. It builds the current proof-of-work target from the configured difficulty.
8. It starts the TCP accept loop on port `9618`.

## 7. Your first code-reading path

If you want to understand the project quickly, read files in this order:

1. `axis/src/main.cpp`
2. `axis/include/axis/blockchain/blockchain.h`
3. `axis/src/blockchain/blockchain.cpp`
4. `axis/include/axis/blockchain/transaction.h`
5. `axis/src/blockchain/transaction.cpp`
6. `axis/include/axis/blockchain/block.h`
7. `axis/src/blockchain/block.cpp`
8. `axis/include/axis/core/common.h`
9. `axis/src/core/common.cpp`
10. `axis/src/storage/database_manager.cpp`

## 8. Mental model of the system

Think of the node as owning four important in-memory collections:

- `blocks`: the ordered blockchain,
- `transactions`: transactions already seen in blocks,
- `transactionsPool`: pending transactions not yet committed to a block,
- `utxo`: currently spendable outputs.

These collections are reconstructed or extended from LevelDB during startup.

## 9. What you can do with the current implementation

You can:

- query all spendable outputs for an address via `GetUTXOs`,
- submit a signed transaction via `CreateTransaction`,
- persist and reload chain and mempool data.

You cannot yet, in the current code, fully:

- mine new blocks automatically,
- synchronize with peers,
- broadcast blocks,
- use a built-in wallet,
- query balances via a completed `GetBalance` path,
- fetch blocks or transactions over fully implemented handlers.

## 10. What beginners often misunderstand

### “Coins” are not separate files or records with identities

In Axis, coins live inside transaction outputs called `UTXO`s. A new transaction spends previous outputs and creates new outputs.

### A transaction hash is not a signature

The transaction hash identifies transaction content. The signature proves the sender authorized it.

### The mempool is not the blockchain

The mempool is only a holding area for valid-but-not-yet-mined transactions.

### LevelDB is not a relational database

The project stores raw key/value pairs. There are no SQL tables.

## 11. Recommended first extension tasks

Good beginner tasks include:

- add a client example for `GetUTXOs`,
- implement a missing payload handler such as `GetTransaction`,
- add tests for malformed packet payloads,
- document exact endianness assumptions in client code,
- add a mining loop that consumes `transactionsPool`.

Before any of those, read:

- [Architecture](architecture.md)
- [Packet protocol](packet_protocol.md)
- [Function reference](function_reference.md)
