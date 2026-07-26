# Design Decisions

## Educational Monolith

Axis keeps all node functionality in one process and one core library. This makes data flow easy to follow and avoids distributed-system scaffolding that would obscure blockchain basics.

## UTXO Accounting

Axis uses UTXOs rather than account balances to demonstrate explicit output ownership, input spending, and double-spend prevention.

## Mempool Spent Index

`pool_spent_` exists so the node can reject or hide local pending double-spends before transactions are mined. This prevents a wallet from selecting an output already reserved by a pending transaction.

## LevelDB Persistence

LevelDB provides simple durable key/value storage without schema migrations or SQL. Blocks and mempool entries are kept in separate databases for clarity.

## Native Binary Format

`Writer`/`Reader` keep serialization compact and easy to inspect. The tradeoff is cross-platform compatibility risk because multi-byte integers are native-endian.

## Manual JSON

HTTP JSON is assembled manually with `std::ostringstream`. This avoids an extra JSON dependency beyond Crow, but it concentrates escaping/formatting responsibility in `src/web.cpp`.

## Split HTTP and TCP APIs

TCP is used for wallet/miner binary workflows. HTTP/WebSocket is used for explorer/dashboard style integration and transaction submission from lightweight clients.

## Current Validation Placement

Transaction validation is in `Chain::add_tx()`. Block submission validation is mostly in `Server::on_create_block()` before calling `Chain::add_block()`. This works for the current small codebase but should be consolidated before adding P2P sync or alternate block ingestion paths.
