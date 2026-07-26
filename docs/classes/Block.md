# `BlockHeader` and `Block`

Source: `include/axis/block.h`, `src/block.cpp`

Blocks group transactions and link to the previous block through the header's `prev_hash`.

## `BlockHeader`

Fields:

| Field | Meaning |
| --- | --- |
| `prev_hash` | Hash of previous block header. Genesis uses zero hash. |
| `merkle_root` | Merkle root of contained transaction txids. |
| `timestamp` | Unix seconds timestamp. |
| `nonce` | Mining nonce. |

Methods:

| Method | Purpose |
| --- | --- |
| `hash()` | Computes Blake2b over serialized header. Used as block ID and proof-of-work hash. |
| `serialize(Writer&)` | Writes the 80-byte header format. |
| `deserialize(Reader&)` | Reads a header from a binary stream. |

## `Block`

Fields:

| Field | Visibility | Meaning |
| --- | --- | --- |
| `header_` | private | Header data. |
| `cached_hash_` | private | Header hash computed at construction/deserialization. |
| `transactions` | public | Full block transactions. |

Constructors and methods:

| Method | Purpose | Notes |
| --- | --- | --- |
| `Block()` | Default construction. | Leaves fields defaulted. |
| `Block(prev, txs, ts, nonce)` | Build a block from transactions. | Computes Merkle root and cached hash. |
| `Block(serialized)` | Restore from storage bytes. | Delegates to `deserialize`. |
| `header()` | Read header. | Returns const reference. |
| `hash()` | Read cached hash. | Returns const reference. |
| `serialize()` | Store full block. | Includes length-prefixed serialized transactions. |
| `deserialize(Reader&)` | Read block from stream. | Recomputes cached hash from header. |

## Invariants

After normal construction:

- `header_.merkle_root == compute_merkle_root(tx.txid() for tx in transactions)`
- `cached_hash_ == header_.hash()`

After deserialization, `cached_hash_` is recomputed, but the Merkle root is not verified against the deserialized transactions.

## Relationships

- `Chain` stores blocks and applies their transactions.
- `Server::on_create_block()` constructs blocks from wire payloads and mempool txs.
- `WebServer` serializes block data into JSON.
- Tests verify block serialization round-trip.

## Performance

- Construction is O(number of transactions) for Merkle leaves plus O(n) Merkle calculation.
- Serialization is O(total serialized transaction bytes).
- Cached hash avoids recomputing header hash for callers of `Block::hash()`.

## Future Improvements

- Make `transactions` immutable/private or add recomputation APIs.
- Verify Merkle root during deserialization/load.
- Add block size and transaction count limits.
