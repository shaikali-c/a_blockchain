# Serialization

Axis uses a custom binary serialization layer built around `Writer` and `Reader` in `include/axis/types.h`. The same primitive helpers are used for object storage, TCP packet payloads, and some HTTP `rawTx` decoding.

## Primitive Rules

| Helper | Bytes | Encoding |
| --- | ---: | --- |
| `put_u8` / `take_u8` | 1 | Raw byte. |
| `put_u16` / `take_u16` | 2 | Native in-memory representation copied with `memcpy`. |
| `put_u32` / `take_u32` | 4 | Native in-memory representation copied with `memcpy`. |
| `put_u64` / `take_u64` | 8 | Native in-memory representation copied with `memcpy`. |
| `put_hash` / `take_hash` | 32 | Raw `Hash` bytes. |
| `put_addr` / `take_addr` | 20 | Raw `Address` bytes. |
| `put_pk` / `take_pk` | `crypto_sign_PUBLICKEYBYTES` | Raw Ed25519 public key bytes. |
| `put_sig` / `take_sig` | `crypto_sign_BYTES` | Raw Ed25519 signature bytes. |
| `put_str` / `take_view(n)` | caller-defined | Raw bytes with no implicit length. |

Multi-byte integers are native-endian. On common x86/x86_64 systems this is little-endian, but the code does not enforce little-endian or network byte order.

## Reader Error Handling

`Reader::check(n)` throws `std::runtime_error("Reader: unexpected end of data")` if a read would pass the end of the buffer. Callers generally catch this at protocol boundaries and convert it to an invalid payload response.

Only some parsers reject trailing bytes:

- `Chain::load_blocks()` rejects trailing bytes after each stored block.
- HTTP `parse_signed_transaction_payload()` rejects trailing bytes.
- TCP `parse_create_tx_payload()` does not check for trailing bytes.
- TCP `parse_create_block_payload()` does not check for trailing bytes.

## Transaction Serialization

`Transaction::serialize()` writes:

```text
txid: 32 bytes
timestamp: u64
input_count: u32
inputs[]:
  txid: 32 bytes
  index: u32
output_count: u32
outputs[]:
  recipient: 20 bytes
  amount: u64
```

This format is used for:

- values in the `pool` LevelDB database,
- transactions nested inside stored blocks,
- test round-trips.

`Transaction::deserialize()` trusts the stored `txid` field and does not recompute or verify that it matches the deserialized inputs/outputs/timestamp.

## Transaction Hash Preimage

`Transaction::compute_hash()` hashes:

1. every input outpoint (`txid`, `index`),
2. every output (`recipient`, `amount`),
3. `timestamp`.

It excludes the stored `txid` and excludes input/output counts. Since hash computation operates on already-separated vectors, the object itself provides boundaries, but the hash preimage is not a self-delimiting canonical byte format.

## Block Header Serialization

`BlockHeader::serialize()` writes exactly 80 bytes:

```text
prev_hash: 32 bytes
merkle_root: 32 bytes
timestamp: u64
nonce: u64
```

`BlockHeader::hash()` is `blake2b(serialized_header)`.

## Block Serialization

`Block::serialize()` writes:

```text
header: 80 bytes
transaction_count: u32
transactions[]:
  tx_size: u32
  tx_bytes: Transaction::serialize() bytes
```

Stored blocks contain full serialized transactions. This differs from the TCP `CreateBlock` request, which carries only transaction IDs for non-coinbase transactions and expects the server to load those transactions from the mempool.

## TCP Packet Envelope

All TCP messages use:

```text
payload_size: u32       // sizeof(MsgType) + payload bytes
msg_type: u16           // raw MsgType enum value
payload: N bytes
```

`Server::send()` writes the envelope with the same native integer representation used elsewhere.

## Compatibility Risks

- Native-endian integers are not portable across different endian architectures.
- There is no protocol version, magic prefix, checksum, compression, or authentication tag.
- Count fields can cause vector reservations; no explicit maximum counts are enforced in most binary parsers.
- `put_str()` writes no length prefix, so every variable field needs an external size.
- Stored transaction deserialization does not verify txid integrity.

See [api/ProtocolPackets.md](api/ProtocolPackets.md) for exact network payload layouts.
