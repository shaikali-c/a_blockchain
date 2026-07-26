# Binary Packet Serialization Layout

This document describes the binary packet and object serialization format currently used by Axis.

Relevant code:

- `include/axis/types.h` — `Writer` / `Reader` primitive encoding helpers
- `include/axis/net.h` — `MsgType` numeric message IDs
- `src/net.cpp` — TCP packet framing and request/response payloads
- `src/tx.cpp` — transaction storage serialization
- `src/block.cpp` — block storage serialization

## Important encoding rules

Axis uses a very small custom binary format. There is no schema version, magic value, checksum, compression, or padding.

### Primitive encoding

| Type/helper | Size | Encoding |
|------------|------|----------|
| `u8` | 1 byte | Raw byte |
| `u16` | 2 bytes | Raw in-memory representation |
| `u32` | 4 bytes | Raw in-memory representation |
| `u64` | 8 bytes | Raw in-memory representation |
| `Hash` | 32 bytes | Raw bytes |
| `Address` | 20 bytes | Raw bytes |
| `PublicKey` | `crypto_sign_PUBLICKEYBYTES` bytes, normally 32 | Raw libsodium Ed25519 public key bytes |
| `Signature` | `crypto_sign_BYTES` bytes, normally 64 | Raw libsodium Ed25519 signature bytes |
| string/view bytes | variable | Raw bytes, length must be encoded by caller if needed |

`Writer` appends integer bytes using `memcpy`-style raw memory writes, and `Reader` reads them the same way. That means multi-byte integers are currently **native-endian**, not explicitly network-endian. On the usual x86/x86_64 target this is little-endian, but the code does not enforce that for cross-platform interoperability.

### No implicit lengths

`put_str()` and `put_span()` do **not** write a length prefix. Any variable-length byte sequence must have its length written explicitly by the surrounding structure.

### Bounds checking

`Reader::check()` throws `std::runtime_error("Reader: unexpected end of data")` when a read would pass the end of the provided buffer.

## TCP packet envelope

All network messages use the same outer frame:

```text
offset  size  field
------  ----  -----
0       4     payload_size: u32
4       2     msg_type: MsgType/u16
6       N     payload bytes
```

Where:

- `payload_size` is the number of bytes after itself: `sizeof(MsgType) + payload.size()`.
- `msg_type` is a `MsgType` enum value encoded as raw `uint16_t` bytes.
- The server rejects packets where `payload_size < sizeof(MsgType)`.

In code, `Server::send()` builds packets as:

```text
[u32 total_size][u16 type][payload]
```

And `Server::handle_client()` reads:

1. `u32 payload_size`
2. exactly `payload_size` bytes
3. first 2 bytes as `MsgType`
4. remaining bytes as message payload

## Message type IDs

Defined in `include/axis/net.h`:

| Value | `MsgType` |
|------:|-----------|
| 0 | `GetUTXOs` |
| 1 | `GetBlock` |
| 2 | `GetTip` |
| 3 | `GetTransaction` |
| 4 | `GetUTXO` |
| 5 | `GetDifficulty` |
| 6 | `GetPool` |
| 7 | `CreateTransaction` |
| 8 | `CreateBlock` |
| 9 | `DifficultyResponse` |
| 10 | `TransactionResponse` |
| 11 | `CreateBlockResponse` |
| 12 | `UTXOsResponse` |
| 13 | `TipResponse` |
| 14 | `PoolResponse` |

Some enum values exist but are not currently handled by `Server::handle_msg()`.

## Network payload layouts

The following sections describe the payload bytes after the packet envelope's `msg_type` field.

### `GetUTXOs` request, type `0`

The server reads the first 20 bytes as an address if present:

```text
offset  size  field
------  ----  -----
0       20    address: Address
```

If fewer than 20 bytes are provided, the server uses the zero address because `addr` is default-initialized and only copied when `payload.size() >= 20`.

### `UTXOsResponse`, type `12`

Produced by `Server::serialize_utxo_response()`:

```text
offset  size        field
------  ----------  -----
0       4           count: u32
4       count * 44  repeated UTXO entries
```

Each UTXO entry:

```text
offset  size  field
------  ----  -----
0       32    txid: Hash
32      4     output_index: u32
36      8     amount: u64
```

Entry size: `32 + 4 + 8 = 44` bytes.

### `GetTip` request, type `2`

No payload is required. The handler ignores any supplied payload.

### `TipResponse`, type `13`

Produced by `Server::on_get_tip()`:

```text
offset  size  field
------  ----  -----
0       32    tip_hash: Hash
```

### `GetPool` request, type `6`

No payload is required. The handler ignores any supplied payload.

### `PoolResponse`, type `14`

Produced by `Server::on_get_pool()`:

```text
offset  size        field
------  ----------  -----
0       4           tx_count: u32
4       tx_count*32 repeated txid: Hash
```

### `GetDifficulty` request, type `5`

No payload is required. The handler ignores any supplied payload.

### `DifficultyResponse`, type `9`

Produced by `Server::on_get_difficulty()`:

```text
offset  size  field
------  ----  -----
0       1     difficulty: u8
```

### `CreateTransaction` request, type `7`

Parsed by `parse_create_tx_payload()`:

```text
offset  size                 field
------  -------------------  -----
0       32                   pubkey: PublicKey
32      8                    timestamp: u64
40      4                    input_count: u32
44      input_count * 36     inputs
...     4                    output_count: u32
...     output_count * 28    outputs
...     64                   signature: Signature
```

Each input is an `OutPoint`:

```text
offset  size  field
------  ----  -----
0       32    txid: Hash
32      4     output_index: u32
```

Input size: `32 + 4 = 36` bytes.

Each output is a `TxOutput`:

```text
offset  size  field
------  ----  -----
0       20    recipient: Address
20      8     amount: u64
```

Output size: `20 + 8 = 28` bytes.

The request does not include a serialized `Transaction` object. Instead, the server reconstructs a `Transaction` from the parsed inputs, outputs, and timestamp, then wraps it with the provided public key and signature as a `SignedTransaction`.

### `TransactionResponse`, type `10`

Produced by `Server::serialize_tx_response()`:

```text
offset  size        field
------  ----------  -----
0       1           success: u8, `1` when `TxError::None`, else `0`
1       1           error_code: TxError/u8
2       2           reason_len: u16
4       reason_len  reason bytes, no terminator
```

`TxError` values from `include/axis/types.h`:

| Value | `TxError` |
|------:|-----------|
| 0 | `None` |
| 1 | `InvalidPayload` |
| 2 | `BadPubkey` |
| 3 | `ZeroAmount` |
| 4 | `BadOwnership` |
| 5 | `BadSignature` |
| 6 | `Duplicate` |
| 7 | `InputSpent` |
| 8 | `Internal` |

### `CreateBlock` request, type `8`

Parsed by `parse_create_block_payload()`:

```text
offset  size             field
------  ---------------  -----
0       32               prev_hash: Hash
32      32               merkle_root: Hash
64      8                timestamp: u64
72      8                nonce: u64
80      20               coinbase_recipient: Address
100     8                coinbase_reward: u64
108     8                coinbase_timestamp: u64
116     4                tx_count: u32
120     tx_count * 32    txids: Hash[]
```

The block request does not include full non-coinbase transactions. It includes only transaction IDs. For every submitted `txid`, the server requires that transaction to already exist in the transaction pool:

```cpp
if (!chain.pool_contains(txid)) {
    return std::unexpected("tx not in pool");
}
```

The server constructs the final block transaction list as:

1. a generated coinbase transaction from `coinbase_recipient`, `coinbase_reward`, and `coinbase_timestamp`
2. each pool transaction referenced by the submitted `txids`

After construction, the server compares the computed block Merkle root against the wire `merkle_root` field.

### `CreateBlockResponse`, type `11`

Produced by the internal `serialize_block_response()` helper:

```text
offset  size        field
------  ----------  -----
0       1           success: u8, `1` when `BlockError::None`, else `0`
1       1           error_code: BlockError/u8
2       2           reason_len: u16
4       reason_len  reason bytes, no terminator
```

`BlockError` values from `include/axis/types.h`:

| Value | `BlockError` |
|------:|--------------|
| 0 | `None` |
| 1 | `InvalidHeight` |
| 2 | `BadPreviousHash` |
| 3 | `InvalidBlockHash` |
| 4 | `HighHash` |
| 5 | `TimeTooFar` |
| 6 | `TimeTooOld` |
| 7 | `BadSignature` |
| 8 | `MissingInputs` |
| 9 | `Duplicate` |
| 10 | `InvalidPayload` |
| 11 | `Internal` |

## Stored / internal object serialization

These layouts are used by `Transaction::serialize()` and `Block::serialize()`. They are related to the wire format but not always identical to request payloads.

### `Transaction::serialize()`

Implemented in `src/tx.cpp`:

```text
offset  size                field
------  ------------------  -----
0       32                  txid: Hash
32      8                   timestamp: u64
40      4                   input_count: u32
44      input_count * 36    inputs: OutPoint[]
...     4                   output_count: u32
...     output_count * 28   outputs: TxOutput[]
```

Each `OutPoint`:

```text
32 bytes  previous txid: Hash
4 bytes   output index: u32
```

Each `TxOutput`:

```text
20 bytes  recipient address: Address
8 bytes   amount: u64
```

Minimum serialized transaction size with zero inputs and zero outputs:

```text
32 txid + 8 timestamp + 4 input_count + 4 output_count = 48 bytes
```

A coinbase transaction is represented as a transaction with zero inputs.

### Transaction hash preimage

`Transaction::compute_hash()` does **not** hash the same byte sequence as `Transaction::serialize()`. The hash preimage is:

```text
for each input:
    txid: Hash
    output_index: u32
for each output:
    recipient: Address
    amount: u64
timestamp: u64
```

Notably, the preimage excludes:

- the `txid` itself
- `input_count`
- `output_count`

Because counts are omitted, the hash remains unambiguous only as long as the transaction object already provides the input/output boundaries before hashing.

### `BlockHeader::serialize()`

Implemented in `src/block.cpp`:

```text
offset  size  field
------  ----  -----
0       32    prev_hash: Hash
32      32    merkle_root: Hash
64      8     timestamp: u64
72      8     nonce: u64
```

Total header size: `80` bytes.

`BlockHeader::hash()` is `blake2b(BlockHeader::serialize())`.

### `Block::serialize()`

Implemented in `src/block.cpp`:

```text
offset  size       field
------  ---------  -----
0       80         block header
80      4          tx_count: u32
84      variable   repeated length-prefixed transactions
```

Each serialized transaction entry inside a block:

```text
offset  size     field
------  -------  -----
0       4        tx_size: u32
4       tx_size  Transaction::serialize() bytes
```

Unlike the `CreateBlock` network request, stored block serialization contains full serialized transactions, not just transaction IDs.

## Quick reference: common sizes

| Item | Size |
|------|-----:|
| Packet length prefix | 4 bytes |
| Message type | 2 bytes |
| Packet envelope overhead | 6 bytes |
| `Hash` | 32 bytes |
| `Address` | 20 bytes |
| `PublicKey` | 32 bytes with libsodium Ed25519 |
| `Signature` | 64 bytes with libsodium Ed25519 |
| `OutPoint` | 36 bytes |
| `TxOutput` | 28 bytes |
| `BlockHeader` | 80 bytes |
| UTXO response entry | 44 bytes |

## Compatibility notes

If this format is used outside the current C++ process/platform, consider standardizing the following before relying on it as a public protocol:

1. Encode all multi-byte integers explicitly as little-endian or big-endian.
2. Add a protocol version or magic value to the packet envelope.
3. Define maximum counts and payload sizes before allocating vectors.
4. Reject trailing bytes in payloads where exact layouts are expected.
5. Decide whether transaction hashes should include input/output counts for canonical serialization.
