# Blockchain

`Chain` is the central blockchain state owner. It stores a linear vector of `Block` objects, a current height, a fixed proof-of-work target, a UTXO map, a pending transaction map, and LevelDB handles.

## Chain Ownership

| Field | Meaning |
| --- | --- |
| `blocks_` | Ordered in-memory vector of blocks. Index `0` is genesis. |
| `height_` | Number of blocks in `blocks_`, not zero-based tip height. |
| `difficulty_` | Hardcoded leading-zero byte count, initialized to `3`. |
| `target_` | 32-byte target hash built by setting first `difficulty_` bytes to `0x00` and the rest to `0xff`. |
| `utxo_` | Current unspent outputs. Key is `OutPoint`, value is `TxOutput`. |
| `pool_` | Pending transactions keyed by txid hash. |
| `pool_spent_` | Pending-input reservation index keyed by outpoint. |
| `blocks_db_` | LevelDB database for persisted blocks. |
| `pool_db_` | LevelDB database for persisted pending transactions. |

## Genesis Block

Genesis is created only when `load_blocks()` finds no blocks.

Hardcoded genesis values:

| Field | Value |
| --- | --- |
| Previous hash | 32 zero bytes |
| Recipient | `f45a20e043b01f65638a46831ce79b8fec3f6737` |
| Amount | `15 * 1,000,000` base units = `15.000000 AXIS` |
| Transaction timestamp | `1781545365` |
| Block timestamp | `1781545365` |
| Nonce | `31496` |

The genesis transaction is a coinbase transaction because it has no inputs.

## Block Storage and Lookup

Blocks are stored in memory in `blocks_` and persisted in LevelDB.

Public lookup methods:

| Method | Behavior |
| --- | --- |
| `tip()` | Returns a copy of `blocks_.back()`. Assumes chain is non-empty. |
| `tip_hash()` | Returns `blocks_.back().hash()`. Assumes chain is non-empty. |
| `height()` | Returns `height_`, the number of blocks. |
| `get_block(uint32_t height)` | Returns a block copy by vector index or `nullopt`. |
| `get_block(const Hash& hash)` | Linear scans blocks and returns `(height, block)` or `nullopt`. |
| `get_blocks(start, count)` | Returns up to `count` block copies from vector index `start`. |

## Block Insertion

`Chain::add_block(const Block& blk)` mutates state and persists an already-accepted block. It does not perform full validation itself.

Steps:

1. Acquire unique lock.
2. For every transaction in the block, call `apply_tx(tx)`.
3. For coinbase transactions, skip mempool cleanup.
4. For non-coinbase transactions:
   - erase each spent input from `pool_spent_`,
   - erase the transaction from `pool_`,
   - delete the transaction from `pool_db_` using its txid hex key.
5. Store the block in `blocks_db_` under the next height key.
6. Push the block into `blocks_`.
7. Increment `height_`.

Because `apply_tx()` is unconditional, callers must not pass invalid blocks to `add_block()`.

## `apply_tx()` and UTXO Mutation

`apply_tx()` implements the UTXO transition for a transaction:

1. Remove every input outpoint from `utxo_`.
2. Insert every output as a new `OutPoint{tx.txid(), index}` mapped to the output.

Coinbase transactions have no inputs, so they only create outputs.

## Chain Validation

Current chain validation is partial:

- Startup loading validates only deserialization and trailing bytes.
- TCP block submission validates Merkle root, hash target, and previous hash before calling `add_block()`.
- `Chain::verify_block_header()` checks previous hash, timestamp greater than tip timestamp, and leading zero bytes, but is not called by the current submission path.
- `Chain::verify_tx()` and `Chain::verify_block()` are declared in `chain.h` but not implemented.

## Blockchain Impact of Operations

| Operation | Blockchain impact |
| --- | --- |
| Transaction accepted to mempool | No block/UTXO mutation, but UTXOs spent by pending inputs are hidden from `get_utxos()`. |
| Block accepted | Permanently mutates UTXO set, removes mined txs from mempool, stores block, increments height. |
| HTTP block reads | No mutation. |
| Startup load | Reconstructs in-memory blockchain state from LevelDB. |

## Design Intent

The chain is deliberately simple and educational: a single local node with a linear append-only chain. The code demonstrates how block persistence, UTXO accounting, mempool double-spend prevention, and basic proof-of-work checks fit together without implementing distributed consensus.
