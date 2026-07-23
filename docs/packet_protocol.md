# Packet Protocol

This document defines the binary network packet format used by Axis.

## 1. Overview

Axis uses a custom TCP binary framing protocol.

Every message sent over the socket is wrapped in a packet with:

1. a size prefix,
2. a payload type,
3. a payload body.

## 2. Top-level packet layout

```text
[size: uint32_t][payloadType: uint16_t][payload bytes...]
```

### Meaning of each field

- `size`: number of bytes that follow after the size field. In other words, `sizeof(payloadType) + payload.size()`.
- `payloadType`: enum identifying what the payload means.
- `payload`: bytes interpreted according to the payload type.

## 3. Code locations

- `Packet` is defined in `axis/include/axis/core/common.h`
- request parsing is handled in `axis/src/blockchain/blockchain.cpp`

## 4. Payload types

Defined in `PayloadType`.

| Enum value | Intended meaning | Current implementation status |
|---|---|---|
| `GetBalance` | request balance information | declared, not implemented |
| `GetBlock` | request block data | declared, not implemented |
| `GetTransaction` | request transaction data | declared, not implemented |
| `GetUTXO` | request one UTXO | declared, handler call commented out |
| `GetUTXOs` | request all UTXOs for an address | implemented |
| `BalanceResponse` | response for balance query | declared only |
| `BlockResponse` | response for block query | declared only |
| `TransactionResponse` | response for transaction submission | implemented |
| `UTXOResponse` | response for one UTXO query | declared only |
| `UTXOsResponse` | response for address UTXO query | implemented |
| `CreateTransaction` | submit signed transaction | implemented |

## 5. Packet construction

`Packet::getPacket()` builds the byte sequence by:

1. computing total `size`,
2. allocating a vector for `sizeof(size) + size`,
3. copying `size`,
4. copying `payloadType`,
5. copying the payload bytes.

## 6. Packet parsing on the server

`Blockchain::readMessage()`:

1. reads `uint32_t payloadSize`,
2. checks it is at least `sizeof(PayloadType)`,
3. reads that many bytes,
4. copies out the `PayloadType`,
5. passes the remaining bytes to `handlePayload()`.

## 7. `GetUTXOs` request

### Purpose

Ask the node for all spendable outputs owned by an address.

### Payload layout

```text
[address: 20 bytes]
```

### Server behavior

- decodes the address,
- scans the in-memory UTXO set,
- returns matching inputs plus total coin amount.

## 8. `UTXOsResponse`

### Payload layout

```text
[inputCount: uint32_t]
repeat inputCount times:
    [transaction_hash: 32]
    [output_index: uint32_t]
[totalCoins: uint64_t]
```

### Meaning

The response contains:

- references to all matching spendable outputs,
- the total sum of those outputs.

### Why inputs are returned instead of full UTXOs

The client mainly needs references it can later spend. The total balance is provided separately.

## 9. `CreateTransaction` request

### Purpose

Submit a signed transaction candidate to the mempool.

### Payload layout

```text
[publicKey]
[sender]
[receiver]
[amount: uint64_t]
[timestamp: uint64_t]
[inputCount: uint32_t]
[inputs...]
[outputCount: uint32_t]
[outputs...]
[signature]
```

More explicitly:

```text
[publicKey: crypto_sign_PUBLICKEYBYTES]
[sender: 20]
[receiver: 20]
[amount: uint64_t]
[timestamp: uint64_t]
[inputCount: uint32_t]
repeat inputCount times:
    [input.transaction_hash: 32]
    [input.output_index: uint32_t]
[outputCount: uint32_t]
repeat outputCount times:
    [output.owner: 20]
    [output.coins: uint64_t]
[signature: crypto_sign_BYTES]
```

## 10. `TransactionResponse`

### Payload layout

```text
[accepted: uint8_t]
[errorCode: uint8_t]
[reasonLength: uint16_t]
[reasonBytes: variable]
```

### Fields

- `accepted`: `1` for accepted, `0` for rejected
- `errorCode`: value from `TransactionErrorCode`
- `reasonLength`: length of human-readable reason
- `reasonBytes`: UTF-8-compatible reason text

## 11. Transaction error codes

Defined in `TransactionErrorCode`.

| Code | Meaning |
|---|---|
| `None` | no error |
| `InvalidPayload` | request bytes could not be parsed |
| `SenderPublicKeyMismatch` | sender address does not match supplied public key |
| `InvalidAmount` | amount was zero |
| `OwnershipVerificationFailed` | inputs missing, duplicated, not owned, or insufficient |
| `SignatureVerificationFailed` | signature did not verify |
| `AlreadyInMempool` | transaction hash already present |
| `InputReservedByMempool` | another pending transaction already uses an input |
| `InternalError` | persistence or internal processing failed |

## 12. Example packet: `GetUTXOs`

Conceptually:

```text
size = 2 + 20 = 22 bytes
payloadType = GetUTXOs
payload = 20-byte address
```

Final frame:

```text
[4 bytes size=22][2 bytes payloadType][20 bytes address]
```

## 13. Example packet: `CreateTransaction`

Conceptually:

```text
[4 bytes size]
[2 bytes payloadType = CreateTransaction]
[32 bytes public key]
[20 bytes sender]
[20 bytes receiver]
[8 bytes amount]
[8 bytes timestamp]
[4 bytes input count]
[... inputs ...]
[4 bytes output count]
[... outputs ...]
[64 bytes signature]
```

Actual `publicKey` and `signature` sizes depend on libsodium constants, but with Ed25519 they are typically 32 and 64 bytes respectively.

## 14. Validation rules that affect protocol users

Clients must ensure:

- sender address is derived from the submitted public key,
- transaction hash is computed over the exact same fields as the server,
- signature signs that exact hash,
- input/output counts match the actual data present,
- there are no trailing bytes.

## 15. Compatibility caveat

Like the rest of Axis serialization, this protocol uses native raw integer layout. That means interoperability across different architectures is not guaranteed unless both ends share the same assumptions.

## 16. Advice for writing packet tests

When testing clients against Axis:

- create golden byte fixtures,
- test malformed counts,
- test truncated packets,
- test trailing-byte rejection,
- test every rejection code path.
