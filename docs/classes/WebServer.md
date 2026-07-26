# `WebServer` HTTP/WebSocket Component

Source: `include/axis/web.h`, `src/web.cpp`

`WebServer` exposes a Crow HTTP API and a WebSocket event stream over shared `Chain` state.

## Responsibilities

- Register REST routes.
- Parse HTTP input.
- Convert chain data to JSON.
- Accept transaction submissions via hex-encoded binary payloads.
- Track WebSocket connections.
- Broadcast accepted transaction/block events.

It should not mutate chain internals directly or own LevelDB handles.

## Public Interface

| Method | Purpose |
| --- | --- |
| `WebServer(Chain&, uint16_t port = 8080)` | Store chain reference/port and call `setup_routes()`. |
| `run()` | Start Crow on configured port with `.multithreaded()`. |
| `stop()` | Stop the Crow app. |
| `broadcast_new_tx(tx)` | Serialize and broadcast a `new_tx` event. |
| `broadcast_new_block(block)` | Serialize and broadcast a `new_block` event. |

## Private Interface

| Method | Purpose |
| --- | --- |
| `setup_routes()` | Registers all routes and WebSocket callbacks. |
| `broadcast_text(message)` | Sends a text message to all stored WebSocket connections. |

## Important Helpers in `src/web.cpp`

| Helper | Purpose |
| --- | --- |
| `json_escape()` | Escapes JSON string content. |
| `json_response()` | Builds Crow response with JSON and CORS headers. |
| `error_response()` | Builds structured error JSON. |
| `parse_u32()` | Strict unsigned 32-bit decimal parser. |
| `hex_to_bytes()` | Decode arbitrary even-length hex payload. |
| `parse_signed_transaction_payload()` | Decode HTTP `rawTx` bytes into `SignedTransaction`; rejects trailing bytes. |
| `transaction_json()` / `block_json()` | Manual JSON serializers. |
| `event_new_tx_json()` / `event_new_block_json()` | WebSocket event JSON serializers. |

## Thread Safety

`ws_connections_` is guarded by `ws_mutex_`. `Chain` methods lock internally. Broadcast sends happen while holding `ws_mutex_`.

## Limitations

- Manual JSON construction rather than a JSON serialization library.
- Permissive CORS.
- No auth or rate limiting.
- No block submission route.
- No WebSocket subscriptions or filtering.
