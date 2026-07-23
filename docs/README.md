# Axis Documentation

Welcome to the complete documentation set for `Axis`, an educational C++ blockchain node project.

This documentation is written for readers who:

- know basic C++ syntax,
- do **not** know this codebase,
- may know little or nothing about blockchain systems,
- want to understand how the current implementation works,
- want to safely extend the project.

> Important: Axis is a **learning-oriented blockchain foundation**, not a production cryptocurrency. The code implements a small proof-of-work chain model, a UTXO set, transaction validation, persistent storage, and a compact binary TCP protocol. Some concepts are present in the design or API surface but are only partially implemented in the current source tree.

## Documentation map

### Start here

1. [Getting started](getting_started.md)
2. [Project structure](project_structure.md)
3. [Architecture](architecture.md)
4. [Blockchain concepts in this project](blockchain.md)

### Core concepts

- [UTXO model](utxo_model.md)
- [Cryptography](cryptography.md)
- [Serialization](serialization.md)
- [Packet protocol](packet_protocol.md)
- [Network](network.md)
- [Database](database.md)

### Runtime workflows

- [Transaction lifecycle](transaction_lifecycle.md)
- [Block lifecycle](block_lifecycle.md)

### Code reference

- [Class reference](class_reference.md)
- [Function reference](function_reference.md)

### Engineering guidance

- [Developer guide](developer_guide.md)
- [Design decisions](design_decisions.md)
- [FAQ](faq.md)
- [Glossary](glossary.md)

## What Axis currently implements

Axis currently includes:

- a singleton `Blockchain` service,
- block and transaction types,
- transaction hashing,
- Merkle root calculation,
- Ed25519 signature verification through libsodium,
- address derivation from public keys,
- an in-memory UTXO set,
- an in-memory mempool plus LevelDB-backed persistence,
- a LevelDB-backed block store,
- a TCP server on port `9618`,
- binary request/response handling for `GetUTXOs` and `CreateTransaction`,
- serialization round-trip tests.

## What Axis does not fully implement yet

The codebase includes traces of broader blockchain ambitions, but these are not complete today:

- no wallet CLI or GUI,
- no key generation flow in the repository,
- no mining loop,
- no peer discovery,
- no peer-to-peer chain synchronization,
- no block broadcast or block acceptance network message,
- no HTTP API despite some shared helper templates referencing Crow,
- no difficulty retargeting,
- several declared handlers are not implemented in the current source tree.

These limitations are important for maintainers, because the documentation must describe the system **as it exists**, not as a fully finished cryptocurrency.

## Recommended reading order for a new contributor

If you are brand new:

1. Read [Glossary](glossary.md).
2. Read [Blockchain](blockchain.md).
3. Read [UTXO model](utxo_model.md).
4. Read [Architecture](architecture.md).
5. Read [Transaction lifecycle](transaction_lifecycle.md).
6. Read [Packet protocol](packet_protocol.md).
7. Read [Class reference](class_reference.md).
8. Read [Function reference](function_reference.md).
9. Read [Developer guide](developer_guide.md).

## Source code areas referenced most often

- `axis/src/main.cpp`
- `axis/include/axis/blockchain/blockchain.h`
- `axis/src/blockchain/blockchain.cpp`
- `axis/include/axis/blockchain/transaction.h`
- `axis/src/blockchain/transaction.cpp`
- `axis/include/axis/blockchain/block.h`
- `axis/src/blockchain/block.cpp`
- `axis/include/axis/core/common.h`
- `axis/src/core/common.cpp`
- `axis/include/axis/storage/database_manager.h`
- `axis/src/storage/database_manager.cpp`
- `axis/include/axis/crypto/cryptography.h`
- `axis/src/crypto/cryptography.cpp`

## How to use this documentation while coding

- Use [Project structure](project_structure.md) when locating code.
- Use [Architecture](architecture.md) when reasoning about module interactions.
- Use [Class reference](class_reference.md) and [Function reference](function_reference.md) while editing code.
- Use [Packet protocol](packet_protocol.md) when building clients or tests.
- Use [Design decisions](design_decisions.md) before refactoring behavior.

## Documentation principles used here

This documentation intentionally:

- explains concepts from first principles,
- connects theory to the exact code in Axis,
- distinguishes current behavior from future-looking intent,
- calls out assumptions and tradeoffs,
- avoids pretending unfinished features are complete.
