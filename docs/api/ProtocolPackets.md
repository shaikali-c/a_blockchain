# Protocol Packets

This is the canonical binary packet layout reference for the current TCP protocol.

## Primitive Encoding

All integers are copied as native in-memory bytes. Fixed byte arrays are raw bytes.

| Type | Size |
| --- | ---: |
| `u8` | 1 |
| `u16` | 2 |
| `u32` | 4 |
| `u64` | 8 |
| `Hash` | 32 |
| `Address` | 20 |
| `PublicKey` | `crypto_sign_PUBLICKEYBYTES` (normally 32) |
| `Signature` | `crypto_sign_BYTES` (normally 64) |

## Envelope

Every TCP message:

```text
offset  size  field
0       4     payload_size = 2 + payload bytes
4       2     msg_type u16
6       N     payload
```

## Message IDs

| Value | Name |
| ---: | --- |
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

Only request values 0, 2, 5, 6, 7, and 8 are handled.

## `GetUTXOs` Request (`0`)

Payload:

```text
address: 20 bytes
```

If fewer than 20 bytes are supplied, the server uses a zero address because it only copies when `payload.size() >= 20`.

## `UTXOsResponse` (`12`)

Payload:

```text
count: u32
entries[count]:
  txid: Hash
  index: u32
  amount: u64
```

Entry size is 44 bytes.

## `GetTip` Request (`2`)

Payload ignored.

## `TipResponse` (`13`)

Payload:

```text
tip_hash: Hash
```

## `GetPool` Request (`6`)

Payload ignored.

## `PoolResponse` (`14`)

Payload:

```text
tx_count: u32
txids[tx_count]: Hash
```

Ordering follows `Chain::get_pool_txs()` copy order from `unordered_map` iteration.

## `GetDifficulty` Request (`5`)

Payload ignored.

## `DifficultyResponse` (`9`)

Payload:

```text
difficulty: u8
```

## `CreateTransaction` Request (`7`)

Payload parsed by `parse_create_tx_payload()`:

```text
pubkey: PublicKey
timestamp: u64
input_count: u32
inputs[input_count]:
  txid: Hash
  index: u32
output_count: u32
outputs[output_count]:
  recipient: Address
  amount: u64
signature: Signature
```

The server reconstructs `Transaction{inputs, outputs, timestamp}` and verifies `signature` over the computed txid.

Trailing bytes are not rejected by the TCP parser.

## `TransactionResponse` (`10`)

Payload:

```text
success: u8     // 1 when TxError::None, else 0
error_code: u8  // TxError numeric value
reason_len: u16
reason: reason_len raw bytes
```

`TxError` numeric values:

| Value | Name |
| ---: | --- |
| 0 | `None` |
| 1 | `InvalidPayload` |
| 2 | `BadPubkey` |
| 3 | `ZeroAmount` |
| 4 | `BadOwnership` |
| 5 | `BadSignature` |
| 6 | `Duplicate` |
| 7 | `InputSpent` |
| 8 | `Internal` |

## `CreateBlock` Request (`8`)

Payload parsed by `parse_create_block_payload()`:

```text
prev_hash: Hash
wire_merkle: Hash
timestamp: u64
nonce: u64
coinbase_recipient: Address
coinbase_reward: u64
coinbase_timestamp: u64
tx_count: u32
txids[tx_count]: Hash
```

The server constructs final block transactions as:

1. generated coinbase transaction from coinbase fields,
2. each mempool transaction referenced by submitted txids.

Every txid must exist in the mempool at parse time. Full transaction bytes are not sent in this request.

Trailing bytes are not rejected by the TCP parser.

## `CreateBlockResponse` (`11`)

Payload:

```text
success: u8      // 1 when BlockError::None, else 0
error_code: u8   // BlockError numeric value
reason_len: u16
reason: reason_len raw bytes
```

`BlockError` numeric values:

| Value | Name |
| ---: | --- |
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

## Stored Object Layouts

Stored transaction and block layouts are documented in [Serialization.md](../Serialization.md). Remember that stored blocks contain full serialized transactions, while `CreateBlock` contains txids only.
