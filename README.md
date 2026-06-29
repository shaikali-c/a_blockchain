# Axis
![Homepage](https://github.com/shaikali-c/a_blockchain/blob/316c8b5a7843fbfaa5b6da96a551b4fc69509504/screenshots/working.png)
The above output screen illustrates the functioning of the developed blockchain wallet application. Through the CLI, the user can create and manage keys, generate and sign transactions, verify digital signatures, and check account balance. This demonstrates the successful implementation of essential wallet-side blockchain operations.


A lightweight, proof-of-work blockchain implementation written in C++. Axis features a UTXO-based transaction model, digital signatures, persistent storage via LevelDB, and a REST API powered by Crow.

## Overview
Axis is a fully functional blockchain node that can:
- Validate and store blocks and transactions
- Maintain a UTXO set
- Verify cryptographic signatures (Ed25519 via libsodium)
- Accept transactions into a mempool
- Accept mined blocks via an API endpoint
- Persist all chain data and the transaction pool to disk using LevelDB
- Serve chain state and data over a REST API


## Features

- Proof-of-Work consensus with configurable difficulty
- UTXO-based accounting for transaction validation
- Ed25519 digital signatures for transaction authorization
- Merkle tree root verification for block integrity
- Transaction mempool with input double-spend protection
- Persistent storage using LevelDB for blocks and pending transactions
- REST API built with Crow for easy integration
- CLI inspection tools (list blocks, transactions, UTXOs, pool)
- Deterministic serialization/deserialization for on-disk storage
- Singleton node instance by design

## Dependencies

| Library              | Purpose |
| :---------------- | :------- |
| C++20        |   Language standard   |
| nlohmann/json           |   JSON parsing and serialization   |
| libsodium    |  Cryptography (Ed25519 signatures, hashing primitives)   |
| LevelDB |  Persistent key-value storage   |
| Crow |  Lightweight HTTP server / REST API   | 

## Project Structure

```
Axis/
├── src/
│   ├── blockchain.cpp        # Core blockchain logic & API handlers
│   ├── block.cpp             # Block structures
│   ├── transaction.cpp       # Transaction, UTXO, Input structures
│   ├── cryptography.cpp      # Hashing, signing, verification, Merkle root
│   ├── utils.cpp             # Helpers (hex/bytes, serialization utils)
│   ├── db.cpp                # LevelDB wrapper
│   ├── common.cpp            # Most used functions
|   └── main.cpp              # Entry point / CLI
├── include/
│   ├── blockchain.h
│   ├── block.h
│   ├── transaction.h
│   ├── cryptography.h
│   ├── utils.h
│   ├── databaseManager.h
│   └── common.h
└── README.md
```

## Transaction Model

Axis uses an Unspent Transaction Output (UTXO) model:

   - UTXO: { owner (address), coins } tied to an output index of a specific transaction.
   - Input: References a UTXO via { transaction_hash, output_index }.
   - Transaction: Contains sender, receiver, amount, inputs, outputs, timestamp, and a computed transaction_hash.
   - SignedTransaction: Wraps the transaction with the sender's public key and Ed25519 signature.

To spend coins, inputs must reference valid, unspent UTXOs owned by the signer. Outputs define new UTXOs distributed to recipients (and any change).

## Genesis Block

The genesis block is hardcoded to ensure all nodes start from the same chain origin. It contains:
    - previousHash of all zeros
    - Precomputed hash and merkleRoot satisfying difficulty
    - A single coinbase transaction awarding GENESIS_REWARD to a fixed address
    - Fixed nonce and timestamp

This block is created automatically on first run if the chain database is empty.

## How it works

On startup, the node:

1. Loads all blocks from blocksDB
2. Rebuilds transactions, utxo, and blocksMap from loaded blocks
3. Loads pending transactions from poolsDB into the mempool
4. Creates the genesis block if the chain is empty
5. Builds the PoW target

## Endpoints

| Method              | Purpose | Description |
| :--- | :------------------------ | :---------------------------------------|
| GET  |  /chain                   | Get current chain summary |
| GET  |  /blocks/\<hash>          | Get block details by hash |
| GET  |  /transactions/\<hash>    | Get transaction details by hash |
| POST |  /utxo                    | Get UTXOs for an address (to construct a transaction) |
| POST |  /createTransaction       | Submit a signed transaction to the mempool |
| POST |  /validateBlock           | Submit a mined block for validation and inclusion |
---
## Current Limitations
- Difficulty is currently static (TODO: dynamic adjustment)
- UTXO lookups are O(n) (TODO: maintain a sorted/optimized structure)
- No peer-to-peer networking yet
