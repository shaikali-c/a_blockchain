# Packet protocol

This document describes Axis's binary packet format and message types.

## Packet format

Every message sent over TCP follows this structure:

```
[magic (4 bytes)] [type (1 byte)] [payload_length (4 bytes)] [payload (N bytes)]
```

| Field | Type | Description |
|-------|------|-------------|
| `magic` | `uint32_t` | Always `0xDEADBEEF`. Used to confirm we're talking to an Axis node. |
| `type` | `uint8_t` | Message type (see below). |
| `payload_length` | `uint32_t` | Length of payload in bytes (big-endian, unsigned). |
| `payload` | `uint8_t[N]` | Type-specific payload. |

Total header size: **9 bytes**.

## Message types

```cpp
enum class MsgType : uint8_t {
    FullBlocks     = 1,   // Blocks 0..N in order (full serialization)
    GetUTXOs       = 3,   // Request: address (20 bytes)
    SendUTXOs      = 4,   // Response: utxo_count (4) + OutPoint[] + TxOutput[]
    GetTx          = 5,   // Request: txid (32 bytes)
    SendTx         = 6,   // Response: serialized SignedTransaction
    GetBlock       = 7,   // Request: block_height (8 bytes as string)  [dead]
    SendBlock      = 8,   // Response: serialized Block                 [dead]
    GetBlockRange  = 9,   // Request: start_range (4), end_range (4)    [dead]
    SendBlockRange = 10,  // Response: blocks_count (4) + blocks[]
    GetMempoolTx   = 11,  // Request: txid (32 bytes)
    CreateTransaction = 12, // SignedTransaction payload
    TransactionResponse = 13, // {accepted (1), err_code (1), reason_len (2), reason (N)}
};
```

> Note: Types 7 (`GetBlock`) and 8 (`SendBlock`) exist in the enum but are
> not handled by the server. Type 9 (`GetBlockRange`) is also not handled.
> These are remnants available for future use.

## Detailed message layouts

### GetUTXOs (type 3)

**Payload:**
```
[address (20 bytes)]
```

Request all UTXOs belonging to a given address.

### SendUTXOs (type 4)

**Payload:**
```
[utxo_count: varint] [OutPoint[0]] [TxOutput[0]] [OutPoint[1]] [TxOutput[1]] ...
```

Where:
- `OutPoint` = `[txid (32)] [index (4)]`
- `TxOutput` = `[recipient (20)] [amount (8)]`
- Count is encoded as a **varint** (see Variable integer encoding).

### CreateTransaction (type 12)

**Payload:**
```
[pubkey (32)] [timestamp (8)] [input_count (4)] [inputs...] [output_count (4)] [outputs...] [signature (64)]
```

Each input:   `[txid (32)] [index (4)]`
Each output:  `[address (20)] [amount (8)]`

### TransactionResponse (type 13)

**Payload:**
```
[accepted (1)] [error_code (1)] [reason_length (2)] [reason (N)]
```

| Field | Type | Description |
|-------|------|-------------|
| `accepted` | `uint8_t` | `1` = accepted, `0` = rejected |
| `error_code` | `uint8_t` | `TxError` code (0 = no error) |
| `reason_length` | `uint16_t` | Length of the reason string |
| `reason` | `uint8_t[N]` | Human-readable reason (UTF-8) |

### SendBlockRange (type 10)

**Payload:**
```
[blocks_count (4)] [blocks_data...]
```

Each block is serialized as:
```
[prev_hash (32)] [merkle_root (32)] [timestamp (8)] [nonce (4)] [version (4)]
[tx_count (4)] [Transaction[0]] [Transaction[1]] ...
```

Each Transaction is serialized as:
```
[txid (32)] [input_count (4)] [inputs...] [output_count (4)] [outputs...] [timestamp (8)]
```

Where each input is `[txid (32)] [index (4)]` and each output is `[address (20)] [amount (8)]`.

### FullBlocks (type 1)

Same format as `SendBlockRange` but with all blocks from height 0 upward.
Sent on initial connection.

### GetTx (type 5)

**Payload:**
```
[txid (32 bytes)]
```

### SendTx (type 6)

**Payload:**
```
[same as CreateTransaction payload format]
```

Serialized `SignedTransaction`: `[pubkey (32)] [timestamp (8)] [input_count (4)] [inputs...] [output_count (4)] [outputs...] [signature (64)]`

### GetMempoolTx (type 11)

**Payload:**
```
[txid (32 bytes)]
```

## Variable integer encoding (varint)

Axis uses a simple variable-length integer encoding to minimize space when
transmitting small values:

| Value range | Encoding | Bytes |
|-------------|----------|-------|
| 0 – 0xFC | Single byte | 1 |
| 0xFD – 0xFFFF | `0xFD` + uint16 LE | 3 |
| 0x10000 – 0xFFFFFFFF | `0xFE` + uint32 LE | 5 |
| ≥ 0x100000000 | `0xFF` + uint64 LE | 9 |

Same encoding as Bitcoin's CompactSize.

### Encoding:

```cpp
// Writer handles this:
void Writer::put_varint(uint64_t v) {
    if (v <= 0xFC) {
        put_u8(v);
    } else if (v <= 0xFFFF) {
        put_u8(0xFD);
        put_u16(v);
    } else if (v <= 0xFFFFFFFF) {
        put_u8(0xFE);
        put_u32(v);
    } else {
        put_u8(0xFF);
        put_u64(v);
    }
}
```

### Decoding:

```cpp
uint64_t Reader::take_varint() {
    auto first = take_u8();
    if (first <= 0xFC) return first;
    if (first == 0xFD) return take_u16();
    if (first == 0xFE) return take_u32();
    return take_u64();
}
```

## Endianness

All multi-byte fields are **little-endian** (LE), except the header's
`payload_length` which is **big-endian** (network byte order).

| Field | Endianness |
|-------|------------|
| `magic` (header) | LE |
| `payload_length` (header) | **BE** (network byte order) |
| `index` (OutPoint) | LE |
| `timestamp` | LE |
| `amount` | LE |
| `nonce` (block) | LE |
| `version` (block) | LE |
| Output count | LE |
| Input count | LE |

## Server dispatch

The `Server::session` coroutine reads packets and dispatches to handlers:

```cpp
auto magic = r.take_u32();
if (magic != 0xDEADBEEF) break;

auto type = r.take_u8();
auto payload_len = r.take_u32_be();  // BE in header

// Read payload bytes
auto payload = co_await async_read(sock, payload_len);

switch (MsgType(type)) {
    case MsgType::GetUTXOs:         co_await on_get_utxos(sock, payload); break;
    case MsgType::CreateTransaction: co_await on_create_tx(sock, payload); break;
    case MsgType::GetTx:            co_await on_get_tx(sock, payload); break;
    case MsgType::GetMempoolTx:     co_await on_get_mempool_tx(sock, payload); break;
    case MsgType::GetBlockRange:    co_await on_get_block_range(sock, payload); break;
    default: break; // ignore unknown
}
```

## Future: FullBlocks on connect

The enum includes `FullBlocks (1)` for sending the entire chain to a new
peer. The handler exists but is not called automatically on connection.
In a future version, handshake logic would trigger this.
