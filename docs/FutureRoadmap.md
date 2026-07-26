# Future Roadmap

This roadmap lists realistic improvements based on current implementation gaps. It is not a claim that these features exist today.

## Correctness and Validation

- Implement `Chain::verify_tx()` or remove the unused declaration.
- Implement `Chain::verify_block()` and make `Chain::add_block()` validate or clearly rename it to `connect_block_unchecked()`.
- Enforce coinbase reward rules using `MINER_REWARD` or remove the unused constant.
- Revalidate mempool transactions when loading from disk.
- Verify stored transaction txid against recomputed hash during deserialization or load.
- Reject trailing bytes consistently in all TCP parsers.
- Add max counts for inputs, outputs, block txids, and payload bytes.

## Consensus

- Add block timestamp windows.
- Add cumulative-work fork choice.
- Add chain reorganization and UTXO undo data.
- Add difficulty retargeting.
- Add block and transaction size limits.
- Add transaction fee accounting and miner fee collection.

## Networking

- Add peer-to-peer discovery and synchronization.
- Add request/response correlation IDs.
- Add protocol version and network magic.
- Add structured error responses for unknown TCP messages.
- Add TLS or document reverse-proxy deployment.
- Add authentication/rate limiting for HTTP APIs.

## Storage

- Add schema version metadata.
- Use LevelDB write batches for atomic block/pool updates.
- Add block hash and address UTXO indexes.
- Store UTXO snapshots for faster startup.
- Store signed transaction metadata if mempool signatures must survive restart.

## Developer Experience

- Move JSON serialization to a dedicated module.
- Split `WebServer::setup_routes()` into smaller functions.
- Add unit tests for validation failure cases.
- Add integration tests for TCP and HTTP APIs.
- Add fuzz tests for binary parsers.
- Add configuration file or command-line flags.

## Security

- Domain-separate hashes.
- Add chain/network ID into transaction signatures.
- Harden payload limits and parsing.
- Avoid manual JSON where possible.
- Avoid sending WebSocket data while holding the connection mutex.

## Product Features

- Wallet CLI or library.
- Miner CLI or mining loop.
- Explorer frontend backed by the HTTP/WebSocket API.
- Peer status and node metrics endpoints.
