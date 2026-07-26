# Database Component

There is no `Database` class in the current implementation. Database ownership and operations live inside `Chain`.

## Actual Implementation

Source: `include/axis/chain.h`, `src/chain.cpp`

Fields:

- `std::unique_ptr<leveldb::DB> blocks_db_`
- `std::unique_ptr<leveldb::DB> pool_db_`

Functions that interact with the database:

| Function | Purpose |
| --- | --- |
| `Chain::Chain()` | Opens both databases. |
| `Chain::load_blocks()` | Reads all block records. |
| `Chain::load_pool()` | Reads all pending transaction records. |
| `Chain::store_block()` | Writes a block record. |
| `Chain::add_tx()` | Writes a pool transaction record. |
| `Chain::add_block()` | Deletes mined pool records and writes a block. |
| `Chain::block_key()` | Formats block height keys. |

## Why There Is No Separate Class

For the current educational code size, persistence is tightly coupled to chain state reconstruction and mutation. Keeping it in `Chain` avoids extra abstraction, but it also means storage logic, validation, and state mutation are mixed.

## Future Extraction Boundary

A future `Database` component could own:

- LevelDB open/close,
- block record put/get/iteration,
- pool record put/delete/iteration,
- schema version metadata,
- write batches,
- corruption/repair helpers.

`Chain` would then depend on a storage interface instead of LevelDB directly.
