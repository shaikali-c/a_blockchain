# Public API

Axis exposes a binary TCP API, an HTTP/JSON API, and a WebSocket event stream.

## Network Ports

| API | Current port | Source |
| --- | ---: | --- |
| TCP binary protocol | `8889` | `src/main.cpp` |
| HTTP/JSON | `8080` | `src/main.cpp` |
| WebSocket | `8080` | `src/main.cpp` |

## HTTP Common Rules

All HTTP route responses are built by `json_response()` and include:

```http
Content-Type: application/json
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, OPTIONS
Access-Control-Allow-Headers: Content-Type, X-API-Key
```

Errors have shape:

```json
{"error":"description","code":400}
```

## REST Endpoints

### `GET /api/status`

Returns node status.

Response fields:

| Field | Type | Source |
| --- | --- | --- |
| `status` | string | literal `online` |
| `version` | string | literal `0.1` |
| `blockHeight` | number | `Chain::height()` |
| `difficulty` | number | `Chain::get_difficulty()` |
| `tipHash` | string | `Chain::tip_hash()` hex |

### `GET /api/tip`

Returns the current tip block using `block_json(chain_.tip(), height - 1)`. If `height() == 0`, returns 404. In normal operation genesis ensures height is at least 1.

### `GET /api/block/<id>`

`<id>` may be:

- unsigned integer height parsed by `parse_u32()`, or
- 32-byte hash hex parsed by `from_hex<32>()`.

Returns 404 if no matching block exists. Returns 400 if the id is neither a valid height nor a valid hash.

### `GET /api/blocks?start=<u32>&count=<u32>`

Returns:

```json
{"blocks":[...],"total":1}
```

Defaults:

- `start = 0`
- `count = 10`

`count` is clamped to `100`.

### `GET /api/mempool`

Returns pending transactions from `Chain::get_pool_txs()`:

```json
{"size":N,"txids":[...],"transactions":[...]}
```

Ordering follows `unordered_map` iteration order and is not stable.

### `GET /api/utxos/<address>`

`address` must be 20-byte hex. Returns:

```json
{"address":"...","balance":15000000,"utxos":[{"txid":"...","index":0,"amount":15000000}]}
```

The response excludes UTXOs reserved by pending mempool transactions.

### `GET /api/address/<address>`

Alias for `/api/utxos/<address>`.

### `POST /api/transaction`

Request body:

```json
{"rawTx":"hex-encoded-create-transaction-payload"}
```

`rawTx` is not `Transaction::serialize()`. It is the binary `CreateTransaction` payload encoded as hex:

```text
pubkey || timestamp || inputs || outputs || signature
```

If accepted, response is:

```json
{"txid":"...","status":"submitted"}
```

On acceptance, WebSocket `new_tx` is broadcast.

### `OPTIONS /api/<path>`

CORS preflight. Returns status `204` with configured headers.

## WebSocket API

Endpoint:

```text
/ws/events
```

### Server Events

| Type | When |
| --- | --- |
| `connected` | immediately on open |
| `new_tx` | transaction accepted through TCP or HTTP |
| `new_block` | block accepted through TCP |
| `pong` | text `ping` or JSON `{"type":"ping"}` received |

There is no subscription filtering; all connected clients receive all broadcast events.

## TCP Public API

Handled request messages:

| Request | Response | Purpose |
| --- | --- | --- |
| `GetUTXOs` | `UTXOsResponse` | UTXO lookup by address. |
| `GetTip` | `TipResponse` | Current tip hash. |
| `GetPool` | `PoolResponse` | Pending txids. |
| `GetDifficulty` | `DifficultyResponse` | Current difficulty byte count. |
| `CreateTransaction` | `TransactionResponse` | Submit signed transaction. |
| `CreateBlock` | `CreateBlockResponse` | Submit mined block. |

See [ProtocolPackets.md](ProtocolPackets.md) for exact layouts.
