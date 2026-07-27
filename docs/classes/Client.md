# Client Component

There is no C++ `Client` class in this repository. Wallet and mining clients are implemented in companion repositories that speak either:

- the binary TCP protocol on port `8889`, or
- the HTTP/WebSocket API on port `8080`.

## TCP Client Responsibilities

A wallet/miner TCP client must:

- encode packets as `u32 payload_size + u16 MsgType + payload`,
- match Axis native integer encoding,
- query UTXOs for addresses,
- construct transaction inputs/outputs,
- compute transaction txid the same way Axis does,
- sign txid with Ed25519,
- submit `CreateTransaction`,
- query pool/difficulty/tip if mining,
- submit `CreateBlock` with coinbase fields and selected pool txids.

## HTTP/WebSocket Client Responsibilities

An explorer/dashboard client can:

- fetch `/api/status`, `/api/blocks`, `/api/tip`, `/api/mempool`, and `/api/utxos/<address>`,
- submit signed transactions with `POST /api/transaction`,
- subscribe to `/ws/events` for `new_tx` and `new_block` notifications.
