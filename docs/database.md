# Database

This document describes how Axis uses LevelDB for persistent storage.

## Overview

Axis uses [LevelDB](https://github.com/google/leveldb) as its embedded
database engine. LevelDB is an open-source key-value store from Google with
the following properties:

- **Embedded**: No separate server process. Just link the library.
- **Ordered**: Keys are stored in sorted order (by byte string comparison).
- **Compressed**: Data blocks are compressed with Snappy by default.
- **Atomic writes**: A `WriteBatch` can apply multiple changes atomically.
- **Single writer**: Only one process can open a database at a time.

## Databases

Axis creates two LevelDB databases under the node's data directory:

| Database | Directory | Contents |
|----------|-----------|----------|
| Blocks | `blocks/` | Serialized blocks + chain tip/height metadata |
| Mempool | `pool/` | Pending (unconfirmed) transactions |

### Data directory

By default, databases are stored in `./axis_data/`. This is hardcoded in
`Chain::Chain()`:

```cpp
Chain::Chain()
    : blocks_db_(open_db("blocks")), pool_db_(open_db("pool")) { ... }
```

### Opening a database

```cpp
static std::unique_ptr<leveldb::DB> open_db(
    const std::string& name) {
    leveldb::DB* db = nullptr;
    leveldb::Options opts;
    opts.create_if_missing = true;
    auto status = leveldb::DB::Open(
        opts, "./axis_data/" + name, &db);
    // status.ok() or throw
    return std::unique_ptr<leveldb::DB>(db);
}
```

RAII ensures automatic cleanup: when `Chain` is destroyed, the `unique_ptr`
destructors close the databases.

## Key-value layout

### Blocks database

The blocks database stores chain metadata and block data:

| Key | Value |
|-----|-------|
| `[-1 (uint8)] [0x00 (uint8)]` | `tip_hash` (Hash, 32 bytes) |
| `[-2 (uint8)]` | `height` (uint64_t, 8 bytes LE) |
| `[height (8 bytes BE)]` | Serialized Block (variable) |

The sentinel keys `-1` and `-2` use a negative-value first byte so they
sort before any block-height key (which starts at `0x00` for height 0).

Key design rationale:
- Block heights are stored as **big-endian** so LevelDB's byte-ordered
  iteration returns blocks in numeric order (height 0, 1, 2, ...).
- Chain metadata uses sentinel keys that sort first, so a single iterator
  can read both metadata and blocks in the correct order.

**Reading all blocks (startup):**
```cpp
void Chain::load_blocks() {
    auto it = std::unique_ptr<leveldb::Iterator>(
        blocks_db_->NewIterator(leveldb::ReadOptions{}));

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        auto key = it->key();
        if (key.size() != 8) continue;  // skip sentinel keys

        auto val = it->value();
        Block blk{std::span<const uint8_t>{
            (const uint8_t*)val.data(), val.size()}};
        // apply transactions to rebuild UTXO set
        txs_.push_back(std::move(blk));
    }
    it = nullptr;  // destroy iterator before next use
}
```

**Storing a block:**
```cpp
void Chain::store_block(Block& blk) {
    std::vector<uint8_t> raw = blk.serialize();

    // Key: 8-byte big-endian height
    auto key = height_key(height_);
    blocks_db_->Put(leveldb::WriteOptions{}, key, to_string(raw));

    // Update tip
    auto tip_key = std::string("\xff\x00", 2);  // -1 sentinel
    blocks_db_->Put(leveldb::WriteOptions{},
                    tip_key, to_string(blk.hash()));

    // Update height
    auto height_key = std::string("\xfe", 1);  // -2 sentinel
    Writer w; w.put_u64(height_);
    blocks_db_->Put(leveldb::WriteOptions{},
                    height_key, to_string(w.buf));
}
```

### Mempool database

The mempool database is a simple flat map:

| Key | Value |
|-----|-------|
| `[txid (32 bytes)]` | Serialized Transaction (variable) |

**Storing a pending transaction:**
```cpp
std::vector<uint8_t> raw = tx.serialize();
pool_db_->Put(leveldb::WriteOptions{},
              to_string(tx.txid()), to_string(raw));
```

**Loading on startup:**
```cpp
void Chain::load_pool() {
    auto it = std::unique_ptr<leveldb::Iterator>(
        pool_db_->NewIterator(leveldb::ReadOptions{}));

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        auto val = it->value();
        Transaction tx{std::span<const uint8_t>{
            (const uint8_t*)val.data(), val.size()}};
        pool_[tx.txid()] = std::move(tx);
    }
}
```

**Removing confirmed transactions:**
```cpp
for (const auto& tx : blk.transactions()) {
    pool_.erase(tx.txid());
    pool_db_->Delete(leveldb::WriteOptions{},
                     to_string(tx.txid()));
}
```

## Error handling

LevelDB operations can fail. Axis uses minimal error handling:

- **Database open failure**: throws `std::runtime_error` with the LevelDB
  status message. This is fatal and crashes the node.
- **Put/Get/Delete failures**: not checked in the current code. In
  production, these should be checked with `.ok()` on the returned status.

## Helper: string conversion

Axis uses a helper to convert `std::span<uint8_t>` to `std::string` for
LevelDB's `Slice` API:

```cpp
static std::string to_string(std::span<const uint8_t> s) {
    return std::string{(const char*)s.data(), s.size()};
}
```

The reverse (LevelDB value to span) happens during iteration.

## No transactions (LevelDB WriteBatch)

LevelDB supports `WriteBatch` for atomic multi-key updates. Axis does not
use this. A block addition involves three writes (tip, height, block data)
that are not atomic. If the node crashes mid-write, the database could be
in an inconsistent state.

In production, all three writes should be wrapped in a `WriteBatch`:
```cpp
leveldb::WriteBatch batch;
batch.Put(tip_key, ...);
batch.Put(height_key, ...);
batch.Put(block_key, ...);
blocks_db_->Write(leveldb::WriteOptions{}, &batch);
```

## Database directory structure

```
axis_data/
├── blocks/
│   ├── CURRENT
│   ├── LOCK
│   ├── LOG
│   ├── MANIFEST-00000
│   └── ...
└── pool/
    ├── CURRENT
    ├── LOCK
    ├── LOG
    ├── MANIFEST-00000
    └── ...
```

Each directory is a self-contained LevelDB database. `LOCK` prevents
multiple processes from opening the same database.
