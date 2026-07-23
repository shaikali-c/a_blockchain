# Class Reference

This document explains every important class and struct in the Axis codebase.

Because Axis is small, this reference also includes major structs and namespaces that shape the design.

## `BlockHeader`

**Defined in:** `axis/include/axis/blockchain/block.h`

### Purpose

Represents the metadata portion of a block.

### Responsibilities

- store the previous block link,
- store the current block hash,
- store proof-of-work related fields,
- store the Merkle root summarizing transactions.

### Member variables

- `Hash previous_hash`: hash of the previous block.
- `Hash hash`: hash identifier of this block.
- `uint64_t timestamp`: block creation time.
- `uint64_t nonce`: proof-of-work nonce.
- `Hash merkleRoot`: root hash of included transactions.

### Lifecycle

Usually created as part of `Block` construction or block deserialization.

### Design rationale

Separating header data from the transaction list mirrors real blockchain designs, where the header often contains the fields relevant to block identity and proof-of-work.

### Common mistakes

- assuming `hash` is automatically recomputed by `Block`; it is supplied by the caller,
- assuming `merkleRoot` is always caller-supplied; normally `Block` computes it.

## `Block`

**Defined in:** `axis/include/axis/blockchain/block.h`

### Purpose

Represents a blockchain block containing header metadata and a list of transactions.

### Responsibilities

- hold block header fields,
- hold transactions,
- compute Merkle root during normal construction,
- serialize and deserialize block data.

### Important members

- `BlockHeader blockHeader`
- `std::vector<Transaction> transactions`

### Important functions

- constructor from header values and transactions,
- constructor from raw serialized bytes,
- `serialize()`.

### Interactions

- uses `Transaction` because blocks contain transactions,
- uses `Cryptography::computeMerkleRoot()` to derive `merkleRoot`.

### Lifecycle

- created during genesis construction,
- created during block deserialization from DB,
- potentially usable for future mined blocks.

### Typical usage

```cpp
Block block{previousHash, blockHash, timestamp, nonce, txs};
```

### Design rationale

A compact value-type block is easy to store, pass, and serialize.

### Common mistakes

- expecting the constructor to verify proof-of-work,
- expecting the constructor to compute `hash`,
- forgetting the block serialization currently stores `size_t` fields.

## `UTXO`

**Defined in:** `axis/include/axis/blockchain/transaction.h`

### Purpose

Represents one unspent transaction output.

### Responsibilities

- store who owns the output,
- store how many coins it contains,
- serialize and deserialize itself.

### Important members

- `Addr owner`
- `uint64_t coins`

### Interactions

- stored inside `Transaction::outputs`,
- stored in the `Blockchain::utxo` map.

### Typical usage

A transaction creates one or more `UTXO` outputs for recipients or change.

### Common mistakes

- thinking a `UTXO` knows which transaction created it; that identity is external and comes from the map key or input reference.

## `Input`

**Defined in:** `axis/include/axis/blockchain/transaction.h`

### Purpose

Represents a reference to a previously created output.

### Responsibilities

- identify the source transaction,
- identify the output index within that transaction,
- generate the UTXO-key string used by maps,
- serialize and deserialize itself.

### Important members

- `Hash transaction_hash`
- `uint32_t output_index`

### Important functions

- `getUTXOKey()`
- `serialize()`
- constructor from UTXO-key string
- `operator==`

### Interactions

- used in `Transaction::inputs`,
- used in `mempoolInputs`,
- used by `updateUTXO()` to erase spent outputs.

### Design rationale

An input is deliberately minimal. It points to value; it does not carry value.

### Common mistakes

- assuming the input contains the amount being spent; the amount is looked up in the UTXO set.

## `Transaction`

**Defined in:** `axis/include/axis/blockchain/transaction.h`

### Purpose

Represents a transfer of value in UTXO form.

### Responsibilities

- hold sender/receiver metadata,
- hold referenced inputs and newly created outputs,
- compute transaction hash,
- serialize and deserialize transaction data.

### Important members

- `std::vector<Input> inputs`
- `std::vector<UTXO> outputs`
- `Addr sender`
- `Addr receiver`
- `Hash transaction_hash`
- `uint64_t coins`
- `uint64_t timestamp`

### Important functions

- constructors for normal transactions,
- constructor for coinbase-like/miner transaction,
- constructor from serialized bytes,
- `serializeTransaction()`
- `deserializeTransaction()`
- internal `computeTransactionHash()` overloads.

### Interactions

- included in blocks,
- stored in the mempool,
- validated by `Blockchain`,
- hashed and signed externally.

### Lifecycle

- created by clients and reconstructed on server,
- persisted to LevelDB,
- restored from LevelDB or block bytes,
- eventually intended for block inclusion.

### Design rationale

The type keeps the full logical contents required for validation, storage, and hashing without introducing scripting or virtual-machine complexity.

### Common mistakes

- assuming `transaction_hash` is serialized directly; it is recomputed on deserialization,
- assuming `coins` alone defines outputs; outputs must still sum appropriately.

## `SignedTransaction`

**Defined in:** `axis/include/axis/blockchain/transaction.h`

### Purpose

Bundles a `Transaction` with the public key and signature needed for verification.

### Responsibilities

- carry all data required to validate sender identity and authorization.

### Member variables

- `Transaction transaction`
- `PublicKey publicKey`
- `Signature signature`

### Interactions

Used mainly in transaction acceptance paths.

### Design rationale

Separating `Transaction` from `SignedTransaction` keeps the core transaction format independent from one specific transport or authentication wrapper.

## `DatabaseManager`

**Defined in:** `axis/include/axis/storage/database_manager.h`

### Purpose

Small wrapper around a `leveldb::DB` instance.

### Responsibilities

- open the database,
- read a key,
- write a key,
- delete a key.

### Important members

- `std::unique_ptr<leveldb::DB> db`

### Important functions

- constructor with path,
- `loadKey()`
- `saveKey()`
- `remove()`

### Interactions

Owned by `Blockchain` for `blocks/` and `pool/` storage.

### Design rationale

It centralizes error translation from LevelDB status objects to C++ exceptions.

### Common mistakes

- assuming it provides transactions, batching, or schema support; it does not.

## `Blockchain`

**Defined in:** `axis/include/axis/blockchain/blockchain.h`

### Purpose

The central node service and state owner.

### Responsibilities

- own chain state,
- own mempool state,
- own UTXO state,
- load persistent state,
- validate transactions,
- verify blocks,
- manage the TCP server,
- dispatch packet handlers,
- serialize responses.

### Important nested structs

#### `AddressUtxos`

Stores:

- matching spendable inputs for an address,
- total coin sum.

Used as an internal helper for `GetUTXOs` responses.

#### `TransactionRejection`

Stores:

- `TransactionErrorCode code`
- `std::string_view reason`

Used to return structured rejection causes from `acceptTransaction()`.

### Important member variables

- `blocks`
- `height`
- `difficulty`
- `target`
- `transactionsPool`
- `transactions`
- `mempoolInputs`
- `utxo`
- `blocksMap`
- `blocksDB`
- `poolsDB`

### Important functions

- `getInstance()`
- `setupConnection()`
- `acceptTransaction()`
- `verifyInputs()`
- `verifySignature()`
- `verifyBlock()`
- `updateUTXO()`
- `loadBlocks()`
- `loadPoolTransactions()`
- `handlePayload()`
- `handleGetUTXOs()`
- `handleCreateTransaction()`

### Interactions

This class interacts with almost every other major type in the codebase.

### Lifecycle

Singleton, lazily initialized, lives until process exit.

### Design rationale

A single orchestrator class keeps the educational code path easy to follow. The cost is high coupling.

### Common mistakes

- treating it as only chain state when it also handles transport and persistence,
- assuming all declared handlers are implemented,
- overlooking that it is not thread-safe for multi-threaded access.

## `Packet`

**Defined in:** `axis/include/axis/core/common.h`

### Purpose

Represents one framed network packet.

### Responsibilities

- hold payload type and payload bytes,
- parse packet body bytes,
- emit full framed packet bytes with size prefix.

### Member variables

- `PayloadType payloadType`
- `std::vector<unsigned char> payload`

### Important functions

- constructor from type and payload,
- constructor from bytes,
- `getPayloadType()`
- `getPacket()`

### Common mistakes

- confusing packet body with full framed packet; `getPacket()` includes the size prefix.

## `BytesWriter`

**Defined in:** `axis/include/axis/core/common.h`

### Purpose

Append raw bytes to a buffer for serialization.

### Responsibilities

- write raw byte arrays,
- write trivially copyable values,
- return finished buffer.

### Design rationale

A tiny helper avoids repetitive low-level byte append logic throughout the code.

## `BytesReader`

**Defined in:** `axis/include/axis/core/common.h`

### Purpose

Read structured fields from a byte sequence.

### Responsibilities

- maintain current offset,
- bounds-check reads,
- reconstruct numeric values and byte arrays.

### Design rationale

Keeps deserialization readable and defensive.

### Common mistakes

- assuming it converts endianness; it does not.

## `UTXOKey`

**Defined in:** `axis/include/axis/core/common.h`

### Purpose

Structured representation of a text UTXO key.

### Members

- `Hash txHash`
- `uint32_t index`

### Why it exists

It makes `parseUTXOKey()` return a typed result instead of forcing string parsing logic into callers.

## `Logger` namespace

**Defined in:** `axis/include/axis/core/logger.h`

### Purpose

Provide simple console logging helpers.

### Functions

- `log()`
- `debug()`
- `error()`
- `reject()`

### Design rationale

Very lightweight logging is enough for an educational node, though production systems would need richer structured logging.

## `Cryptography` namespace

**Defined in:** `axis/include/axis/crypto/cryptography.h`

### Purpose

Group cryptographic helper functions.

### Current public function

- `computeMerkleRoot()`

### Design rationale

Keeps Merkle-tree behavior logically separated from chain orchestration and transaction code.
