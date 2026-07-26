# `Server` TCP Component

Source: `include/axis/net.h`, `src/net.cpp`

`Server` owns the binary TCP API and uses Asio coroutines for sessions.

## Responsibilities

- Bind and listen on a TCP port.
- Accept client connections.
- Read packet envelopes.
- Dispatch handled `MsgType` requests.
- Parse binary transaction/block payloads.
- Serialize binary responses.
- Call `Chain` for state reads/mutations.
- Trigger event callbacks for accepted txs/blocks.

It should not own chain state directly, HTTP routes, or WebSocket connections.

## Public Interface

| Method | Purpose |
| --- | --- |
| `Server(Chain&, uint16_t port = 8889, ServerEvents events = {})` | Construct acceptor on IPv4 endpoint and keep chain/event references. |
| `run()` | Start accepting and run the Asio event loop. Blocks until `ctx_.run()` returns. |

## Private Interface

| Method | Purpose |
| --- | --- |
| `do_accept()` | Schedules one async accept and re-arms after completion. |
| `handle_client(sock)` | Coroutine reading packets from one socket until error/EOF. |
| `handle_msg(type,payload,sock)` | Switch dispatch for request types. |
| `on_get_utxos()` | Reads optional address payload and returns matching UTXOs. |
| `on_get_difficulty()` | Returns one-byte difficulty. |
| `on_get_tip()` | Returns current tip hash. |
| `on_get_pool()` | Returns pending txids. |
| `on_create_tx()` | Parses signed tx payload, calls `Chain::add_tx()`, broadcasts on accept, returns tx response. |
| `on_create_block()` | Parses block proposal, validates Merkle/PoW/previous hash, calls `Chain::add_block()`, broadcasts on accept. |
| `send()` | Writes response envelope and payload. |
| `serialize_utxo_response()` | Encodes UTXO list. |
| `serialize_tx_response()` | Encodes transaction result. |

## Internal Helpers

- `ParsedCreateTx` and `ParsedCreateBlock` are local structs in `src/net.cpp`.
- `parse_create_tx_payload()` reads the `CreateTransaction` payload.
- `parse_create_block_payload()` reads block fields and resolves txids from the chain mempool.
- `serialize_block_response()` encodes block result payloads.
- `tx_error_str()` maps `TxError` to response text.

## Thread Safety

`Server` runs on the Asio context thread in the current process. It calls `Chain`, whose public methods lock internally. Event callbacks call into `WebServer` for broadcasts.

## Failure Cases

- Bad length smaller than `MsgType` ends session.
- Unknown message type is logged without response.
- Parser exceptions become invalid payload responses for transaction/block submission.
- Socket read/write exceptions end session.

## Future Improvements

- Add max packet size.
- Add request IDs and structured unknown-message errors.
- Reject trailing bytes consistently.
- Move block validation into `Chain`.
