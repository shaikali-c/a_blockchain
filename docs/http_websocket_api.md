# Axis HTTP and WebSocket API

Axis exposes an HTTP/JSON API and a WebSocket event stream alongside the existing binary TCP protocol.

- HTTP base URL: `http://localhost:8080`
- WebSocket URL: `ws://localhost:8080/ws/events`
- Binary TCP node protocol: unchanged, served separately on port `8889`

The HTTP/WebSocket API is intended for explorers, dashboards, web UIs, and lightweight integrations. Wallets/miners that already use the binary protocol can continue using it unchanged.

## Common Response Rules

REST endpoints return JSON with `Content-Type: application/json`.

The current implementation sends permissive CORS headers:

```http
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, OPTIONS
Access-Control-Allow-Headers: Content-Type, X-API-Key
```

For production, restrict `Access-Control-Allow-Origin` to the frontend domain.

### Error Format

Errors return a JSON object:

```json
{
  "error": "description",
  "code": 400
}
```

Common status codes:

| Code | Meaning |
| --- | --- |
| `200` | Request succeeded. |
| `204` | CORS preflight succeeded. |
| `400` | Bad input, invalid JSON, invalid hex, or rejected transaction. |
| `404` | Resource was not found. |
| `413` | Payload too large. |

## Data Formats

### Hash

A hash is encoded as a lowercase 32-byte hex string, 64 characters total.

Example:

```text
6f4c3e...64 hex chars total...
```

### Address

An address is encoded as a lowercase 20-byte hex string, 40 characters total.

Example genesis address:

```text
f45a20e043b01f65638a46831ce79b8fec3f6737
```

### Amount

Amounts are unsigned integers in base units. The display unit is AXIS, where current code uses:

```text
1 AXIS = 1,000,000 base units
```

### Timestamp

Timestamps are Unix timestamps in seconds.

## REST Endpoints

### `GET /api/status`

Returns basic node and chain status.

#### Request

No body.

#### Example

```bash
curl http://localhost:8080/api/status
```

#### Response

```json
{
  "status": "online",
  "version": "0.1",
  "blockHeight": 1,
  "difficulty": 3,
  "tipHash": "<32-byte-hex-hash>"
}
```

#### Fields

| Field | Type | Description |
| --- | --- | --- |
| `status` | string | Current server status. Currently `online`. |
| `version` | string | API/node version string. Currently `0.1`. |
| `blockHeight` | number | Number of blocks in the chain. Genesis-only chain returns `1`. |
| `difficulty` | number | Current proof-of-work difficulty byte count. |
| `tipHash` | string | Hash of the current tip block. |

---

### `GET /api/tip`

Returns the current tip block with full transaction data.

#### Request

No body.

#### Example

```bash
curl http://localhost:8080/api/tip
```

#### Response

```json
{
  "height": 0,
  "hash": "<block-hash>",
  "previousHash": "<previous-block-hash>",
  "merkleRoot": "<merkle-root>",
  "timestamp": 1781545365,
  "nonce": 31496,
  "txids": ["<txid>"],
  "transactions": [
    {
      "txid": "<txid>",
      "timestamp": 1781545365,
      "coinbase": true,
      "inputs": [],
      "outputs": [
        {
          "recipient": "f45a20e043b01f65638a46831ce79b8fec3f6737",
          "amount": 15000000
        }
      ]
    }
  ]
}
```

#### Errors

| Code | Meaning |
| --- | --- |
| `404` | Chain is empty. This should not normally happen because genesis is created automatically. |

---

### `GET /api/block/<id>`

Returns a block by height or hash.

`<id>` can be either:

- unsigned integer block height, for example `0`
- 32-byte hex block hash, 64 hex characters

#### Examples

```bash
curl http://localhost:8080/api/block/0
```

```bash
curl http://localhost:8080/api/block/<32-byte-hex-block-hash>
```

#### Response

Same shape as `GET /api/tip`.

```json
{
  "height": 0,
  "hash": "<block-hash>",
  "previousHash": "<previous-block-hash>",
  "merkleRoot": "<merkle-root>",
  "timestamp": 1781545365,
  "nonce": 31496,
  "txids": ["<txid>"],
  "transactions": []
}
```

#### Errors

| Code | Meaning |
| --- | --- |
| `400` | Block id is neither an unsigned integer nor a valid 32-byte hex hash. |
| `404` | Block not found. |

---

### `GET /api/blocks`

Returns a paginated list of block summaries.

#### Query Parameters

| Parameter | Type | Required | Default | Max | Description |
| --- | --- | --- | --- | --- | --- |
| `start` | unsigned integer | no | `0` | none | First block height to return. |
| `count` | unsigned integer | no | `10` | `100` | Number of blocks to return. Values above `100` are clamped to `100`. |

#### Examples

```bash
curl 'http://localhost:8080/api/blocks'
```

```bash
curl 'http://localhost:8080/api/blocks?start=0&count=25'
```

#### Response

```json
{
  "blocks": [
    {
      "height": 0,
      "hash": "<block-hash>",
      "previousHash": "<previous-block-hash>",
      "merkleRoot": "<merkle-root>",
      "timestamp": 1781545365,
      "nonce": 31496,
      "transactionCount": 1
    }
  ],
  "total": 1
}
```

#### Errors

| Code | Meaning |
| --- | --- |
| `400` | `start` or `count` is not an unsigned integer. |

---

### `GET /api/mempool`

Returns transactions currently waiting in the mempool.

#### Request

No body.

#### Example

```bash
curl http://localhost:8080/api/mempool
```

#### Response

```json
{
  "size": 1,
  "txids": ["<txid>"],
  "transactions": [
    {
      "txid": "<txid>",
      "timestamp": 1781545365,
      "coinbase": false,
      "inputs": [
        {
          "txid": "<previous-txid>",
          "index": 0
        }
      ],
      "outputs": [
        {
          "recipient": "<20-byte-address-hex>",
          "amount": 1000000
        }
      ]
    }
  ]
}
```

---

### `GET /api/utxos/<address>`

Returns unspent outputs for an address.

`<address>` must be a 20-byte hex address, 40 hex characters.

#### Example

```bash
curl http://localhost:8080/api/utxos/f45a20e043b01f65638a46831ce79b8fec3f6737
```

#### Response

```json
{
  "address": "f45a20e043b01f65638a46831ce79b8fec3f6737",
  "balance": 15000000,
  "utxos": [
    {
      "txid": "<txid>",
      "index": 0,
      "amount": 15000000
    }
  ]
}
```

#### Errors

| Code | Meaning |
| --- | --- |
| `400` | Address is not a valid 20-byte hex value. |

---

### `GET /api/address/<address>`

Alias for `GET /api/utxos/<address>`.

Use this endpoint when the frontend wants an address-centric route name.

#### Example

```bash
curl http://localhost:8080/api/address/f45a20e043b01f65638a46831ce79b8fec3f6737
```

#### Response

Same shape as `GET /api/utxos/<address>`.

---

### `POST /api/transaction`

Submits a signed transaction to the node mempool.

Important: `rawTx` is not `Transaction::serialize()` output. It is a hex-encoded binary payload matching the existing TCP `CreateTransaction` payload layout, because transaction submission requires the public key and signature.

#### Request Headers

```http
Content-Type: application/json
```

#### Request Body

```json
{
  "rawTx": "<hex-encoded-create-transaction-payload>"
}
```

#### Binary Payload Layout Before Hex Encoding

The `rawTx` hex string decodes to the following binary fields, in order:

| Field | Type/Size | Description |
| --- | --- | --- |
| `pubkey` | `crypto_sign_PUBLICKEYBYTES` bytes | Sender public key. |
| `timestamp` | `uint64` | Transaction timestamp. |
| `input_count` | `uint32` | Number of transaction inputs. |
| `inputs[].txid` | 32 bytes each | Previous output transaction id. |
| `inputs[].index` | `uint32` each | Previous output index. |
| `output_count` | `uint32` | Number of outputs. |
| `outputs[].recipient` | 20 bytes each | Recipient address. |
| `outputs[].amount` | `uint64` each | Amount in base units. |
| `signature` | `crypto_sign_BYTES` bytes | Signature over the transaction id. |

Integers currently follow the same byte layout as the existing C++ `Writer`/`Reader` helpers.

#### Example

```bash
curl -X POST \
  -H 'Content-Type: application/json' \
  -d '{"rawTx":"<hex-encoded-create-transaction-payload>"}' \
  http://localhost:8080/api/transaction
```

#### Success Response

```json
{
  "txid": "<txid>",
  "status": "submitted"
}
```

When accepted, the server also broadcasts a `new_tx` WebSocket event.

#### Errors

| Code | Meaning |
| --- | --- |
| `400` | Missing `rawTx`, invalid JSON, invalid hex, malformed payload, or transaction rejected by chain validation. |
| `413` | `rawTx` exceeds the configured maximum size. |

Possible transaction rejection messages include:

- `invalid payload`
- `bad public key`
- `zero amount`
- `ownership failed`
- `bad signature`
- `duplicate`
- `input spent`
- `internal error`

---

### `OPTIONS /api/<path>`

CORS preflight endpoint.

#### Example

```bash
curl -X OPTIONS http://localhost:8080/api/status
```

#### Response

Status code `204` with CORS headers.

## WebSocket API

### Connect

Connect to:

```text
ws://localhost:8080/ws/events
```

Browser example:

```js
const ws = new WebSocket('ws://localhost:8080/ws/events');

ws.onopen = () => {
  console.log('Axis WebSocket connected');
};

ws.onmessage = (event) => {
  const message = JSON.parse(event.data);
  console.log('Axis event:', message);
};

ws.onclose = () => {
  console.log('Axis WebSocket closed');
};

ws.onerror = (error) => {
  console.error('Axis WebSocket error:', error);
};
```

Node.js example using `ws`:

```js
import WebSocket from 'ws';

const ws = new WebSocket('ws://localhost:8080/ws/events');

ws.on('message', (data) => {
  console.log(JSON.parse(data.toString()));
});
```

### Client-to-Server Messages

The current WebSocket implementation is mostly server-push.

Supported client message:

#### Ping

Send either plain text:

```text
ping
```

or JSON text:

```json
{"type":"ping"}
```

Server response:

```json
{"type":"pong"}
```

No subscription/filtering protocol is currently implemented. All connected WebSocket clients receive all broadcast events.

### Server-to-Client Messages

#### Connected

Sent immediately after a successful WebSocket connection.

```json
{
  "type": "connected"
}
```

#### New Transaction

Sent when a transaction is accepted through either:

- the HTTP `POST /api/transaction` endpoint
- the existing binary TCP `CreateTransaction` flow

Message shape:

```json
{
  "type": "new_tx",
  "txid": "<txid>",
  "transaction": {
    "txid": "<txid>",
    "timestamp": 1781545365,
    "coinbase": false,
    "inputs": [
      {
        "txid": "<previous-txid>",
        "index": 0
      }
    ],
    "outputs": [
      {
        "recipient": "<20-byte-address-hex>",
        "amount": 1000000
      }
    ]
  }
}
```

#### New Block

Sent when a block is accepted through the existing binary TCP `CreateBlock` flow.

Message shape:

```json
{
  "type": "new_block",
  "hash": "<block-hash>",
  "timestamp": 1781545365,
  "transactionCount": 2
}
```

After receiving `new_block`, clients that need full block details should call one of:

```text
GET /api/tip
GET /api/block/<hash>
```

## Frontend Usage Pattern

A typical explorer/dashboard can:

1. Fetch initial state with REST.
2. Open the WebSocket.
3. Update local UI state when events arrive.
4. Refetch full resources when an event only contains summary data.

Example:

```js
async function loadInitialState() {
  const [status, mempool] = await Promise.all([
    fetch('http://localhost:8080/api/status').then((r) => r.json()),
    fetch('http://localhost:8080/api/mempool').then((r) => r.json()),
  ]);

  return { status, mempool };
}

function subscribeToAxisEvents() {
  const ws = new WebSocket('ws://localhost:8080/ws/events');

  ws.onmessage = async (event) => {
    const message = JSON.parse(event.data);

    switch (message.type) {
      case 'connected':
        console.log('Connected to Axis events');
        break;
      case 'new_tx':
        console.log('New transaction:', message.txid);
        break;
      case 'new_block': {
        const tip = await fetch('http://localhost:8080/api/tip').then((r) => r.json());
        console.log('New tip:', tip);
        break;
      }
      case 'pong':
        console.log('Received pong');
        break;
      default:
        console.warn('Unknown Axis event:', message);
    }
  };

  return ws;
}
```

## Endpoint Summary

| Endpoint | Method | Request Body | Response | Notes |
| --- | --- | --- | --- | --- |
| `/api/status` | `GET` | none | node status | Basic health/status endpoint. |
| `/api/tip` | `GET` | none | full block | Current tip block. |
| `/api/block/<id>` | `GET` | none | full block | `<id>` is height or 32-byte hash. |
| `/api/blocks` | `GET` | none | block summaries | Supports `start` and `count`. |
| `/api/mempool` | `GET` | none | mempool transactions | Includes txids and full transaction objects. |
| `/api/utxos/<address>` | `GET` | none | address UTXOs | `<address>` is 20-byte hex. |
| `/api/address/<address>` | `GET` | none | address UTXOs | Alias for `/api/utxos/<address>`. |
| `/api/transaction` | `POST` | `{ "rawTx": "..." }` | tx submission result | `rawTx` is hex-encoded TCP CreateTransaction payload. |
| `/api/<path>` | `OPTIONS` | none | empty `204` | CORS preflight. |
| `/ws/events` | WebSocket | optional ping | event messages | Broadcasts `new_tx` and `new_block`. |

## Operational Notes

- HTTP server port is currently hardcoded in `src/main.cpp` as `8080`.
- TCP server port is currently hardcoded in `src/main.cpp` as `8889`.
- WebSocket clients are stored in memory and receive broadcast events while connected.
- There is no authentication or rate limiting in the Crow layer yet.
- There is no TLS in-process. For production, run behind a TLS reverse proxy such as Nginx/Caddy, or enable TLS support in Crow.
