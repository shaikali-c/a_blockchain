# Core Types, Reader, Writer, Timestamp

Source: `include/axis/types.h`

## Fixed Types

- `Hash`: 32 bytes.
- `Address`: 20 bytes.
- `PublicKey`: libsodium Ed25519 public key bytes.
- `SecretKey`: libsodium Ed25519 secret key bytes.
- `Signature`: libsodium Ed25519 signature bytes.

## Error Enums

`TxError` and `BlockError` are protocol-facing error enums. Their numeric order is the wire representation.

## `Writer`

`Writer` is a local serialization helper with public `buf`. It appends bytes and never removes them. It has no synchronization and should be stack-local.

## `Reader`

`Reader` wraps a `std::string_view` and an offset. It advances as fields are read. Every `take_*` method checks bounds before reading. It does not automatically check for trailing bytes; callers must compare `offset` to `data.size()` when exact consumption matters.

## `Timestamp`

`Timestamp` wraps `uint64_t value`, supports `now()`, and implements comparison operators. The code stores Unix seconds.

## Design Notes

The serialization helpers are deliberately tiny and dependency-free, but they use native byte order. If Axis becomes a cross-platform protocol, these helpers should be replaced or extended with explicit endian encoding.
