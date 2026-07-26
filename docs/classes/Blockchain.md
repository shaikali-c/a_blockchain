# `Chain` / Blockchain Component

Source: `include/axis/chain.h`, `src/chain.cpp`

`Chain` is the blockchain state owner. It owns block storage, UTXO state, mempool state, proof-of-work parameters, and LevelDB handles.

## Responsibilities

`Chain` owns:

- ordered block vector `blocks_`,
- current height `height_`,
- fixed difficulty and target,
- UTXO map,
- mempool map,
- mempool spent-input map,
- block and pool LevelDB handles.

`Chain` should not own:

- socket I/O,
- HTTP routing,
- JSON formatting,
- WebSocket connection state,
- wallet key storage.

## Public Interface

| Function | Purpose | Behavior |
| --- | --- | --- |
| `Chain()` | Initialize persistent and in-memory chain state. | Opens DBs, loads blocks/pool, creates genesis if needed, logs state, builds target. Throws on DB/load failures. |
| `~Chain()` | Cleanup. | Default destructor closes DB handles via `unique_ptr`. |
| `TxError add_tx(const SignedTransaction&)` | Validate and persist a pending transaction. | Checks amounts, ownership, signature, duplicates, pending input conflicts; writes to pool DB. |
| `get_utxos(addr, outpoints)` | Wallet/explorer lookup. | Appends unspent, non-pool-reserved outpoints for an address. |
| `tip()` | Current block. | Returns copy of last block. Requires non-empty chain. |
| `tip_hash()` | Current block hash. | Returns hash of last block. Requires non-empty chain. |
| `height()` | Chain length. | Returns number of blocks. |
| `get_difficulty()` | Mining parameter. | Returns hardcoded difficulty byte count. |
| `target()` | Mining target. | Returns current target hash. |
| `get_block(height)` | Lookup by index. | O(1), returns optional copy. |
| `get_block(hash)` | Lookup by block hash. | O(n) scan, returns optional height/block pair. |
| `get_blocks(start,count)` | Pagination. | Returns copies in height order. |
| `get_pool_txs()` | Mempool listing. | Returns copies of pending txs. |
| `pool_contains(txid)` | Block parser support. | Tests mempool membership. |
| `get_pool_tx(txid)` | Block parser support. | Returns pending transaction copy or throws `out_of_range`. |
| `add_block(block)` | Connect accepted block. | Applies transactions, removes mined pool entries, persists block, increments height. Does not validate fully. |

## Private Algorithms

- `load_blocks()` iterates LevelDB blocks in key order, deserializes, checks trailing bytes, applies transactions, and fills `blocks_`.
- `load_pool()` iterates persisted pending txs and rebuilds `pool_` and `pool_spent_`.
- `apply_tx()` performs the UTXO state transition.
- `build_target()` converts `difficulty_` into a 32-byte target.
- `create_genesis()` creates the hardcoded first block.
- `block_key()` formats height keys as ten decimal digits.
- `verify_block_header()` checks previous hash, timestamp, and leading zero bytes, but is not currently used.

`verify_tx()` and `verify_block()` are declared but not implemented.

## Thread Safety

Public read methods use `std::shared_lock`. Mutators use `std::unique_lock`. Private helpers rely on construction-time single-threading or caller-held locks.

## Performance

- Address UTXO lookup is O(total UTXOs).
- Hash block lookup is O(total blocks).
- Startup is O(total persisted transactions).
- Block insertion is O(transactions and inputs/outputs in the block).

## Future Improvements

- Make `pool_` private.
- Add block hash and address indexes.
- Validate blocks inside `Chain` before mutation.
- Use LevelDB batches for atomic updates.
- Store/rebuild UTXO snapshots for faster startup.
