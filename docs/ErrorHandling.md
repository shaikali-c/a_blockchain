# Error Handling

Axis uses a mixture of enum return codes, exceptions, and logging.

## Error Enums

### `TxError`

| Value | Meaning in current implementation |
| --- | --- |
| `None` | Transaction accepted. |
| `InvalidPayload` | Malformed payload, empty inputs, arithmetic overflow, or parser failure. |
| `BadPubkey` | Defined but not currently returned by chain validation. |
| `ZeroAmount` | Zero output or zero total output amount. |
| `BadOwnership` | Input missing, sender mismatch, or insufficient input sum. |
| `BadSignature` | Ed25519 verification failed. |
| `Duplicate` | Txid already pending. |
| `InputSpent` | Input already reserved by another pending transaction. |
| `Internal` | Defined for protocol/API mapping but rarely used directly. |

### `BlockError`

| Value | Meaning in current implementation |
| --- | --- |
| `None` | Block accepted. |
| `InvalidBlockHash` | Merkle root mismatch in `on_create_block()`. |
| `HighHash` | Block hash is greater than target. |
| `BadPreviousHash` | Submitted previous hash is not current tip hash. |
| `InvalidPayload` | Malformed payload or txid missing from pool. |
| Others | Defined but not currently emitted by runtime block submission. |

## Exceptions

| Source | Exception behavior |
| --- | --- |
| `Reader::check()` | Throws `runtime_error` on short buffer. |
| `from_hex()` | Throws on invalid length or hex data. |
| LevelDB open | `Chain::Chain()` throws on open failure. |
| LevelDB put/delete | `store_block()`, `add_tx()`, `add_block()` throw on write/delete failure. |
| `load_blocks()` | Wraps deserialization/iteration failures with key context and throws. |

## TCP Error Responses

- `CreateTransaction` catches parser exceptions and sends `TransactionResponse` with `InvalidPayload` and reason text.
- `CreateBlock` catches parser/handler exceptions and sends `CreateBlockResponse` with `InvalidPayload` and reason text.
- Unknown message types are logged without a response.
- Session-level packet errors end the client session.

## HTTP Error Responses

`error_response(code, message)` returns:

```json
{"error":"message","code":400}
```

The message is escaped with `json_escape()`.

Common status codes:

- `400`: bad input, invalid hex, malformed payload, rejected transaction, not parseable block id.
- `404`: missing block or empty chain where applicable.
- `413`: raw transaction hex exceeds maximum size.
- `204`: CORS preflight response body is empty but still uses `json_response()` headers.

## Logging

`logging::info`, `logging::err`, and `logging::reject` print to `std::cout` with prefixes:

- `[INFO]`
- `[ERR]`
- `[REJ]`

There is no log level filtering, no timestamps in log lines, and no separate stderr stream.

## Error Handling Limitations

- Some state mutations happen before LevelDB operations that may throw, so in-memory rollback is not guaranteed.
- Unknown TCP messages do not receive structured protocol errors.
- HTTP transaction rejection maps all `TxError` values to status `400`, except payload too large.
- Block submission catches broad exceptions and reports `InvalidPayload`, even if the underlying issue is storage/internal.
- Startup failures abort the daemon.
