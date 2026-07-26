# Debugging Guide

## Build and Test

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Startup Debugging

Startup path:

1. `sodium_init()`
2. `Chain::Chain()`
3. LevelDB open for `blocks` and `pool`
4. `load_blocks()`
5. `load_pool()`
6. optional `create_genesis()`
7. `build_target()`
8. Crow thread start
9. TCP server start

If startup fails:

- Check whether `blocks/` or `pool/` contains corrupt data.
- Temporarily move those directories to force genesis recreation.
- Inspect thrown error text from `load_blocks()`; it includes the failing key for block deserialization.

## Transaction Rejection Debugging

`Chain::add_tx()` returns a `TxError`. Map it as follows:

| Error | What to inspect |
| --- | --- |
| `ZeroAmount` | Any output amount equals zero or no total output. |
| `InvalidPayload` | Empty inputs, arithmetic overflow, parser failure. |
| `BadOwnership` | Input not in UTXO set, wrong derived address, or insufficient funds. |
| `BadSignature` | Signature not over the computed txid or wrong public key. |
| `Duplicate` | Same txid already in mempool. |
| `InputSpent` | Another pending tx spends the same input. |

Useful checks:

- Derive sender address from the public key and compare to UTXO recipient.
- Recompute txid exactly as Axis does.
- Ensure the client signs the txid, not serialized transaction bytes.
- Query `/api/utxos/<address>` to see available non-reserved outputs.
- Query `/api/mempool` to see pending transactions.

## Block Rejection Debugging

`CreateBlock` can reject with:

| Error | What to inspect |
| --- | --- |
| `InvalidPayload` | Binary layout, missing pool txid, parse failure. |
| `InvalidBlockHash` | Merkle root mismatch. |
| `HighHash` | Nonce/timestamp does not satisfy target. |
| `BadPreviousHash` | Miner built on stale tip. |

Important: server reconstructs the transaction list as coinbase first, then mempool transactions in submitted txid order. The Merkle root must match that exact list.

## Serialization Debugging

- `Reader: unexpected end of data` means a parser tried to read past payload end.
- Check native-endian integer encoding if using a client written in another language.
- Ensure count fields match the number of following entries.
- For HTTP transaction submission, trailing bytes are rejected.
- For TCP transaction/block submission, trailing bytes are currently accepted; do not rely on this long-term.

## Runtime Logs

Axis logs to `std::cout`:

- `[INFO]` informational messages,
- `[ERR]` infrastructure/session errors,
- `[REJ]` validation rejections.

There is no log-level control today.

## Resetting Local Chain State

For a clean local educational run, stop the daemon and remove runtime DB directories:

```bash
rm -rf blocks pool
```

Then restart `axisd`; genesis will be recreated.

## Debugging Threading Issues

- `Chain` methods should be called instead of direct field access.
- WebSocket broadcasts hold `ws_mutex_` while sending; if WebSocket operations hang, inspect connected clients.
- Crow runs multithreaded, so HTTP route handlers may run concurrently.
