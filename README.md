# Blockchain
A custom blockchain implementation in C++ with UTXO-based transactions, persistent storage, and a REST API.

---

## What is this?
A from scratch blockchain built in C++ that handles wallets, transactions, and a UTXO set, all backed by LevelDB and exposed via a HTTP server.

---

## Features

- **UTXO model** : tracks unspent outputs just like Bitcoin does
- **Ed25519 key pairs** : signing and verification via libsodium
- **LevelDB persistence** : UTXOs and transactions survive restarts
- **REST API** : get transactions or create new ones over HTTP
- **Pretty table output** : lists transactions and UTXOs in the terminal nicely (tabulate)
- **Async loading** : UTXO and transaction data loads in parallel on startup

---

## dependencies

| Library | Why |
|---|---|
| [libsodium](https://libsodium.org) | Crypto (hashing, signing, key gen) |
| [LevelDB](https://github.com/google/leveldb) | Persistent key-value storage |
| [Drogon](https://github.com/drogonframework/drogon) | HTTP server / REST API |
| [tabulate](https://github.com/p-ranav/tabulate) | Terminal table formatting |

## project structure

```
├── pch.h               # Precompiled header, all includes live here
├── blockchain.cpp      # Core blockchain logic (transactions, UTXO, routes)
├── keys.cpp            # Key generation, signing, serialization
├── common.cpp          # Utility functions (hex, hashing, base64)
├── databaseManager.cpp # LevelDB wrapper
└── logger.cpp          # Simple console logger
```

---

## How it works

1. On startup, UTXOs and transactions are loaded from LevelDB asynchronously
2. Call `init(owner)` to bootstrap the chain with an initial coin supply
3. Transactions consume UTXOs from the sender and create new ones for the receiver (with change going back if needed)
4. Everything gets persisted to disk automatically
5. The REST API lets you query or create transactions over HTTP

### Endpoints

```
GET  /transactions						— list all transactions
GET  /transaction/{transaction_id}      — get a transaction
POST /create_transaction				— create a new transaction
```

---

## Known issues / todo

- [ ] DB path is hardcoded — needs to be made portable
- [ ] No block structure yet (it's transaction/UTXO focused right now)
- [ ] Blockchain export to a transferable file
- [ ] No P2P networking
- [ ] Windows-only at the moment (`WIN32_LEAN_AND_MEAN` in pch.h)

---
## Notes

This is a learning project. Don't use it in production for anything real,  it's meant to explore how blockchain internals work at a low level.
