# Axis documentation

This directory explains the entire Axis blockchain node from first principles.
You do not need prior blockchain knowledge.

## How to read this documentation

If you are **completely new to blockchain**, start at the top and read down:

1. [Glossary](glossary.md) — define every term before you encounter it
2. [Blockchain concepts](blockchain.md) — what is a blockchain, from zero
3. [UTXO model](utxo_model.md) — how the accounting works
4. [Cryptography](cryptography.md) — hashes, signatures, keys, addresses
5. [Architecture](architecture.md) — how the modules fit together
6. [Project structure](project_structure.md) — where everything lives
7. [Transactions](transaction_lifecycle.md) — from creation to validation
8. [Blocks](block_lifecycle.md) — block structure and genesis
9. [Serialization](serialization.md) — binary format of everything
10. [Packet protocol](packet_protocol.md) — TCP wire format
11. [Networking](network.md) — how the server accepts and handles messages
12. [Database](database.md) — LevelDB key/value storage
13. [Getting started](getting_started.md) — build, run, and explore
14. [Class reference](class_reference.md) — every type in detail
15. [Function reference](function_reference.md) — every function in detail
16. [Developer guide](developer_guide.md) — how to add features
17. [Design decisions](design_decisions.md) — why each choice was made
18. [FAQ](faq.md) — common questions answered

## Quick map

```
Terminal                     Code module                Key concept
─────────────────────────────────────────────────────────────────
./build/axisd          →     main.cpp          Entry point
TCP port 9618          →     net.h / net.cpp   Server, coroutines
GetUTXOs / CreateTx    →     net.cpp           Message handlers
Chain                  →     chain.h / .cpp    Validation, persistence
UTXO set               →     chain.cpp         Unspent outputs
Mempool (tx pool)      →     chain.cpp         Pending transactions
Block                  →     block.h / .cpp    Block structure
Transaction            →     tx.h / .cpp       TX structure
OutPoint / TxOutput    →     tx.h              Input / output
Writer / Reader        →     types.h           Binary serialization
blake2b / Merkle root  →     crypto.h / .cpp   Hashing
Ed25519 sign / verify  →     crypto.h / .cpp   Signatures
LevelDB                →     chain.cpp         Persistent storage
```
