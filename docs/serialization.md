# Serialization format

This document describes how each data type is serialized to bytes for
storage (LevelDB) and network transmission. Axis uses a custom binary
serialization with `Writer` (encoding) and `Reader` (decoding).

## Design principles

1. **Explicit typed methods**: Unlike the old `Writer::put(T val)` template
   (which truncated arrays on GCC), every serialization method is explicit
   about its type: `put_u8`, `put_u32`, `put_u64`, `put_hash`, `put_addr`,
   `put_pk`, `put_sig`.

2. **Little-endian by default**: Multi-byte integers use little-endian
   encoding. Network header `payload_length` is a special exception (big-endian).

3. **Compact but deterministic**: The format is not self-describing. Both
   Writer and Reader must agree on structure.

## Writer

```cpp
struct Writer {
    std::vector<uint8_t> buf;  // grows as data is appended

    void put_u8(uint8_t v);
    void put_u16(uint16_t v);   // little-endian
    void put_u32(uint32_t v);   // little-endian
    void put_u64(uint64_t v);   // little-endian
    void put_u32_be(uint32_t v); // big-endian (for network header)
    void put_varint(uint64_t v);
    void put_hash(const Hash& h);   // hash.data(), 32 bytes
    void put_addr(const Address& a); // addr.data(), 20 bytes
    void put_pk(const PublicKey& pk); // pk.data(), 32 bytes
    void put_sig(const Signature& sig); // sig.data(), 64 bytes
    void put_bytes(std::span<const uint8_t> data); // raw bytes
};
```

### Method reference

| Method | Appends | Use case |
|--------|---------|----------|
| `put_u8` | 1 byte | Small counters, flags |
| `put_u16` | 2 bytes LE | String lengths, small ranges |
| `put_u32` | 4 bytes LE | Counts, indices |
| `put_u64` | 8 bytes LE | Timestamps, amounts |
| `put_u32_be` | 4 bytes BE | Network payload length header |
| `put_varint` | 1–9 bytes | Compact variable-length integer |
| `put_hash` | 32 bytes | txid, block hash, Merkle root |
| `put_addr` | 20 bytes | Addresses |
| `put_pk` | 32 bytes | Ed25519 public keys |
| `put_sig` | 64 bytes | Ed25519 signatures |
| `put_bytes` | N bytes | Raw data passthrough |

## Reader

```cpp
struct Reader {
    std::span<const uint8_t> buf;  // the data to read
    size_t offset = 0;              // current read position

    uint8_t     take_u8();
    uint16_t    take_u16();    // little-endian
    uint32_t    take_u32();    // little-endian
    uint64_t    take_u64();    // little-endian
    uint32_t    take_u32_be(); // big-endian
    uint64_t    take_varint();
    Hash        take_hash();   // 32 bytes
    Address     take_addr();   // 20 bytes
    PublicKey   take_pk();     // 32 bytes
    Signature   take_sig();    // 64 bytes
    std::span<const uint8_t> take_bytes(size_t n); // raw bytes
};
```

Reader advances `offset` after each read. If `offset + size > buf.size()`,
the behavior is undefined (the caller must ensure sufficient data).

## Composite types

### Hash

```
[data (32 bytes)]
```

Stored and transmitted as raw binary (not hex). 32 bytes total.

### Address

```
[data (20 bytes)]
```

A 20-byte Blake2b hash of a public key. 20 bytes total.

### OutPoint

```
[txid (32 bytes)] [index (4 bytes LE)]
```

36 bytes total.

### TxOutput

```
[recipient (20 bytes)] [amount (8 bytes LE)]
```

28 bytes total.

### Transaction

```
[txid (32 bytes)]
[input_count (4 bytes LE)]
[inputs... (count * 36 bytes)]
[output_count (4 bytes LE)]
[outputs... (count * 28 bytes)]
[timestamp (8 bytes LE)]
```

The `txid` is included in the serialization so the reader knows the
transaction hash without recomputing it.

### SignedTransaction

```
[public_key (32 bytes)]
[ts = timestamp (8 bytes LE)]
[input_count (4 bytes LE)]
[inputs... (count * 36 bytes)]
[output_count (4 bytes LE)]
[outputs... (count * 28 bytes)]
[signature (64 bytes)]
```

Note: `SignedTransaction` does NOT store the txid separately — the receiver
derives the address from `public_key`, reconstructs the `Transaction`
object, and verifies the signature.

### Block (header + body)

```
[prev_hash (32 bytes)]
[merkle_root (32 bytes)]
[timestamp (8 bytes LE)]
[nonce (4 bytes LE)]
[version (4 bytes LE)]
[tx_count (4 bytes LE)]
[transactions... (count * variable)]
```

The block hash is NOT stored in the serialization — it is computed on
deserialization by hashing the header (first 80 bytes).

## Block hash computation

The block hash is: `blake2b(header_bytes)` where header_bytes are:

```
[prev_hash (32)] [merkle_root (32)] [timestamp (8)] [nonce (4)] [version (4)]
```

Total header size: **80 bytes**. The hash is 32 bytes.

## Storage format (LevelDB)

LevelDB is a key-value store. Keys and values are byte strings.

### Blocks database (`blocks/` directory)

| Key | Value | Description |
|-----|-------|-------------|
| `[-1 (1 byte)] [0x00 (1 byte)]` | `tip_hash` (32 bytes) | Chain tip hash |
| `[-2 (1 byte)]` | `height` (8 bytes LE) | Chain height |
| `[height (8 bytes BE)]` | Serialized Block | Block at that height |

Block height is stored as **big-endian** so that LevelDB iteration returns
blocks in order (height 0, 1, 2, ...). The sentinel keys use negative-first-
byte so that the chain metadata sorts before any block.

### Mempool database (`pool/` directory)

| Key | Value | Description |
|-----|-------|-------------|
| `[txid (32 bytes)]` | Serialized Transaction | A pending transaction |

The mempool database is a simple flat map of txid → transaction. On startup,
all entries are loaded into memory.
