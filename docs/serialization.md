# Serialization

Serialization is the process of converting in-memory objects into bytes so they can be stored or sent over the network.

Axis relies heavily on manual binary serialization.

## 1. Why serialization is needed

Axis uses serialization in three major places:

1. storing `Transaction` objects in LevelDB,
2. storing `Block` objects in LevelDB,
3. sending and receiving protocol packets over TCP.

Without serialization, C++ objects could not be safely persisted or transmitted as raw structured data.

## 2. Axis approach

Axis uses a lightweight custom binary format rather than JSON, protobuf, or another external encoding system.

### Benefits

- compact,
- fast,
- simple to implement,
- no additional runtime encoding dependency.

### Costs

- easier to get wrong,
- harder to evolve compatibly,
- requires exact agreement on field order and native integer layout.

## 3. Core helper types

Defined in `axis/include/axis/core/common.h`.

## `BytesWriter`

Used to append bytes into a growing buffer.

### Main methods

- `writeBytes(...)`: append a container or array of bytes,
- `writeValues(T value)`: append a trivially copyable value by copying its raw memory bytes.

## `BytesReader`

Used to read fields from a byte string in order.

### Main methods

- `readBytes<T>()` for trivially copyable types,
- `readBytes<S>()` for fixed-size byte arrays,
- `readBytesString(size_t size)` for raw string slices,
- `require(size_t)` to enforce bounds.

## 4. Endianness assumptions

This is one of the most important topics for maintainers.

### What the code does

For integers like `uint32_t`, `uint64_t`, and even `size_t`, Axis copies raw in-memory bytes directly with `memcpy` or byte iteration.

### What that implies

The serialized layout depends on:

- machine endianness,
- machine type sizes,
- ABI details for types like `size_t`.

### Practical consequence

Axis serialization is safest when:

- writer and reader use the same architecture family,
- compiler/platform assumptions match.

### Important risk

This format is **not a portable, versioned wire standard**.

For example:

- `uint64_t` byte order is not explicitly normalized,
- `size_t` may differ between 32-bit and 64-bit systems.

If the project grows into a multi-platform network, this should be refactored.

## 5. Why `memcpy` is used

The code uses `memcpy` because it is a straightforward way to copy raw bytes of trivially copyable values into or out of buffers.

### Benefits

- fast,
- simple,
- avoids undefined behavior from type punning through incompatible references.

### Tradeoff

It still preserves native representation rather than enforcing a canonical external format.

## 6. Packet framing format

A network packet is serialized as:

```text
[size: uint32_t][payloadType: uint16_t][payload bytes...]
```

Where:

- `size` is the byte length of `payloadType + payload`,
- `payloadType` is a `PayloadType` enum value,
- `payload` is message-specific content.

This is implemented by `Packet::getPacket()`.

## 7. `Transaction` binary layout

Serialized by `Transaction::serializeTransaction()`.

Layout:

```text
[sender: 20]
[receiver: 20]
[inputCount: uint32_t]
[inputs...]
[outputCount: uint32_t]
[outputs...]
[coins: uint64_t]
[timestamp: uint64_t]
```

Each input is:

```text
[transaction_hash: 32][output_index: uint32_t]
```

Each output is:

```text
[owner: 20][coins: uint64_t]
```

### Important note

`transaction_hash` itself is **not** stored in the serialized transaction body. It is recomputed during deserialization from the restored fields.

That is good design, because it avoids trusting a transmitted hash field that might not match the contents.

## 8. `Block` binary layout

Serialized by `Block::serialize()`.

Layout:

```text
[previous_hash: 32]
[hash: 32]
[merkleRoot: 32]
[nonce: uint64_t]
[timestamp: uint64_t]
[transactionCount: size_t]
repeat transactionCount times:
    [transactionDataSize: size_t]
    [transactionBytes: variable]
```

### Important portability warning

Using `size_t` in stored block format ties the format to platform word size.

## 9. `CreateTransaction` request layout

Parsed by `Blockchain::deserializeCreateTransactionRequest()`.

Layout:

```text
[publicKey: crypto_sign_PUBLICKEYBYTES]
[sender: 20]
[receiver: 20]
[amount: uint64_t]
[timestamp: uint64_t]
[inputCount: uint32_t]
[inputs...]
[outputCount: uint32_t]
[outputs...]
[signature: crypto_sign_BYTES]
```

### Why timestamp is included

The transaction hash depends on the timestamp. Both client and server must hash the exact same content.

## 10. `UTXOsResponse` layout

Built by `Blockchain::serializeUtxosResponse()`.

Layout:

```text
[inputCount: uint32_t]
repeat inputCount times:
    [transaction_hash: 32]
    [output_index: uint32_t]
[totalCoins: uint64_t]
```

This response gives the client references to spendable outputs and the summed value.

## 11. `TransactionResponse` layout

Built by `Blockchain::serializeTransactionResponse()`.

Layout:

```text
[accepted: uint8_t]
[errorCode: uint8_t]
[reasonLength: uint16_t]
[reasonBytes: variable]
```

## 12. Defensive parsing behavior

Axis includes several useful parsing checks.

### In `BytesReader`

- bounds checks via `require()`.

### In transaction deserialization

- rejects impossible input counts,
- rejects impossible output counts.

### In create-transaction request parsing

- validates there is enough trailing space for required fields,
- rejects trailing extra bytes.

### In packet parsing

- rejects frames smaller than a payload type.

These checks reduce the chance of silent memory misuse.

## 13. Example transaction serialization

Suppose a transaction contains:

- sender address: 20 bytes,
- receiver address: 20 bytes,
- 1 input,
- 2 outputs,
- amount: 42,
- timestamp: 123456789.

The byte layout would conceptually be:

```text
20 bytes sender
20 bytes receiver
4 bytes input count = 1
32 bytes input tx hash
4 bytes input output index
4 bytes output count = 2
20 bytes output0 owner
8 bytes output0 coins
20 bytes output1 owner
8 bytes output1 coins
8 bytes coins = 42
8 bytes timestamp = 123456789
```

## 14. Compatibility recommendations for future work

If you extend Axis seriously, consider these improvements:

1. use explicit little-endian or big-endian encoding helpers,
2. replace `size_t` in on-disk/wire formats with fixed-width integers,
3. version the packet and block formats,
4. add golden-byte serialization tests,
5. define maximum allowed counts and message sizes.

## 15. Summary

Serialization in Axis is simple and efficient, but it depends on native machine layout in several places. It works well as an educational in-process and same-platform design, but it should be hardened before being treated as a stable cross-platform protocol.
