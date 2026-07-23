# Function Reference

This document explains every non-trivial function in the visible Axis source tree.

To keep the reference readable, trivial one-line constructors and very small logging wrappers are grouped where appropriate.

---

# `axis/src/main.cpp`

## `int main()`

### What it does

Program entry point.

### Why it exists

Starts the node process.

### When it is called

By the operating system when the executable launches.

### Who calls it

The OS / runtime loader.

### Behavior

1. calls `sodium_init()`,
2. logs error if libsodium initialization fails,
3. obtains `Blockchain::getInstance()`,
4. calls `setupConnection()`,
5. catches any exception and logs a generic initialization failure,
6. waits on `std::cin.get()` before returning.

### Side effects

- initializes global crypto library state,
- may start TCP server and event loop,
- reads from standard input before exit.

### Common pitfalls

- broad `catch(...)` loses failure detail,
- `std::cin.get()` means the process intentionally waits for console input before exiting.

---

# `axis/src/blockchain/transaction.cpp`

## `std::string Input::getUTXOKey() const`

### What it does

Formats an input reference as `<txHashHex>:<output_index>`.

### Why it exists

The UTXO map uses string keys, so inputs need a stable mapping format.

### Called by

- `Blockchain::verifyInputs()`
- `Blockchain::updateUTXO()`
- mempool reservation logic

### Return value

UTXO key string.

### Complexity

O(1) relative to project data size.

### Pitfalls

Relies on a text format rather than a binary composite key.

## `Input::Input(const std::string& utxoKey)`

### What it does

Parses a text UTXO key into structured `transaction_hash` and `output_index` fields.

### Why it exists

Used when the node needs to rebuild input references from UTXO map keys, especially in address query responses.

### Called by

- `Blockchain::findAddressUtxos()`

### Error handling

Throws if the key format is invalid.

## `bool Input::operator==(const Input& in) const`

### What it does

Compares transaction hash and output index.

### Why it exists

Used by tests and logical equality checks.

## `std::string Input::serialize() const`

### What it does

Serializes the input as transaction hash bytes followed by output index bytes.

### Why it exists

Allows inputs to be embedded in serialized transactions.

## `std::string UTXO::serialize() const`

### What it does

Serializes owner address and coin amount.

### Why it exists

Allows outputs to be embedded in serialized transactions.

## `Transaction::Transaction(const std::string& rawBytes)`

### What it does

Constructs a transaction by deserializing raw bytes.

### Why it exists

Used for loading transactions from LevelDB or block contents.

### Called by

- `Blockchain::loadPoolTransactions()`
- `Block::Block(std::string_view)`
- tests

## `Transaction::Transaction(const Addr& miner, uint64_t c, std::vector<UTXO> o)`

### What it does

Builds a coinbase-like transaction with zero sender and no inputs.

### Why it exists

Supports miner/genesis reward style transactions.

### Side effects

Computes the transaction hash immediately.

## `Transaction::Transaction(const Addr& s, const Addr& r, uint64_t c, std::vector<Input> i, std::vector<UTXO> o)`

### What it does

Builds a normal transaction and computes its hash using the current system time.

### Why it exists

Convenient constructor for locally created transactions.

### Pitfalls

Hash depends on the exact timestamp chosen at construction time.

## `Transaction::Transaction(..., uint64_t t)`

### What it does

Builds a transaction with an explicit timestamp and computes a deterministic hash from that timestamp.

### Why it exists

Necessary when reconstructing a transaction from wire or stored bytes.

## `void Transaction::computeTransactionHash()`

### What it does

Sets `timestamp` to current epoch seconds and delegates to the timestamped overload.

### Why it exists

Provides automatic timestamp assignment for newly created transactions.

### Side effects

Mutates `timestamp` and `transaction_hash`.

### Thread safety

Safe only under normal value-object use; no internal locking.

## `void Transaction::computeTransactionHash(uint64_t t)`

### What it does

Hashes canonical transaction content using the provided timestamp.

### Why it exists

Makes transaction identity deterministic when timestamp is already known.

### Called by

- constructors
- `deserializeTransaction()`

### Parameters

- `t`: timestamp to set and include in the hash.

### Side effects

Mutates `timestamp` and `transaction_hash`.

### Complexity

O(number of inputs + number of outputs).

### Example execution

For each input, append hash and index. For each output, append owner and coins. Then append `coins` and `timestamp`, hash the full buffer.

### Common pitfalls

Any field ordering change changes transaction identity and breaks signature compatibility.

## `std::string Transaction::serializeTransaction() const`

### What it does

Serializes a transaction into raw bytes.

### Why it exists

Needed for DB persistence and block embedding.

### Return value

Binary string representation of the transaction.

### Complexity

O(number of inputs + number of outputs).

## `void Transaction::deserializeTransaction(const std::string& buffer)`

### What it does

Parses a transaction from raw bytes and recomputes its hash.

### Why it exists

Allows safe reconstruction without trusting an externally supplied hash.

### Error handling

Throws on invalid counts or truncated data.

### Side effects

Mutates all transaction fields, reserves vectors, computes hash.

### Pitfalls

Uses native integer layout assumptions through `BytesReader`.

---

# `axis/src/blockchain/block.cpp`

## `Block::Block(const Hash& ph, const Hash& bh, uint64_t t, uint64_t n, const std::vector<Transaction>& txs)`

### What it does

Constructs a block from supplied header fields and transactions, then computes the Merkle root.

### Why it exists

Creates block objects from already prepared data.

### Parameters

- `ph`: previous block hash
- `bh`: block hash
- `t`: timestamp
- `n`: nonce
- `txs`: block transactions

### Side effects

Sets `blockHeader.merkleRoot`.

### Complexity

O(number of transactions) plus Merkle computation cost.

## `Block::Block(std::string_view rawBytes)`

### What it does

Deserializes a block from raw bytes.

### Why it exists

Used for loading persisted blocks.

### Error handling

Throws on invalid counts or insufficient bytes.

### Pitfalls

Uses `size_t` from the serialized stream, so portability is limited.

## `std::string Block::serialize() const`

### What it does

Serializes the block and all embedded transactions.

### Why it exists

Needed for block persistence.

### Complexity

O(number of transactions + total serialized transaction bytes).

---

# `axis/src/core/common.cpp`

## `Addr computeAddress(const PublicKey& pk)`

### What it does

Hashes a public key into a 20-byte address.

### Why it exists

Provides a compact owner identifier derived from cryptographic identity.

### Called by

- `Blockchain::verifyInputs()`
- `Blockchain::handleCreateTransaction()`

### Return value

Derived address.

## `void appendBytes(std::string& buffer, const void* data, size_t size)`

### What it does

Appends raw bytes to a string.

### Why it exists

General-purpose helper, though it is not central to the current main flows.

## `Hash hashBytesVector(const std::vector<unsigned char>& bytes)`

### What it does

Hashes a vector of bytes into a `Hash`.

### Why it exists

Used by transaction hashing code.

## `UTXOKey parseUTXOKey(const std::string& key)`

### What it does

Parses a text key of the form `<hashHex>:<index>`.

### Why it exists

Allows string-based UTXO identifiers to be converted back into structured references.

### Error handling

Throws if the key is malformed.

### Pitfalls

Uses `std::stoul`; extremely large indices or malformed strings throw.

---

# `axis/src/crypto/cryptography.cpp`

## `Hash Cryptography::computeMerkleRoot(const std::vector<Hash>& transactions)`

### What it does

Computes a Merkle root from transaction hashes.

### Why it exists

Blocks need a compact integrity commitment to included transactions.

### Parameters

- list of transaction hashes.

### Return value

Merkle root hash, or zero-filled hash for empty input.

### Complexity

Roughly O(n) hashing work across levels.

### Behavior details

- duplicates last hash if a level has odd size,
- iteratively hashes concatenated pairs until one root remains.

### Common pitfalls

Empty input returns a zero hash, which callers should interpret carefully.

---

# `axis/src/storage/database_manager.cpp`

## `DatabaseManager::DatabaseManager(const std::string& path)`

### What it does

Opens or creates a LevelDB database at `path`.

### Why it exists

Initializes persistent storage wrappers.

### Error handling

Throws `std::runtime_error` if opening fails.

### Side effects

Creates database directories if needed via LevelDB.

## `std::string DatabaseManager::loadKey(const std::string& key) const`

### What it does

Loads a value by key.

### Why it exists

Convenience wrapper for raw reads.

### Return value

Stored value or empty string if not found.

### Error handling

Throws on read failures other than not-found.

## `void DatabaseManager::remove(const std::string& kname) const`

### What it does

Deletes a key from the database.

### Error handling

Throws if delete fails.

## `std::string DatabaseManager::saveKey(const std::string& key, const std::string& value) const`

### What it does

Writes a key/value pair.

### Why it exists

Used for block and mempool persistence.

### Side effects

Mutates persistent DB state.

### Return value

Returns `value` for convenience.

---

# `axis/src/blockchain/blockchain.cpp`

## `Blockchain::Blockchain()`

### What it does

Initializes chain state, opens databases, loads persisted data, creates genesis block if needed, and builds the difficulty target.

### Why it exists

Constructs the singleton node state.

### Side effects

- opens databases,
- reads persistent state,
- may create and persist genesis block.

## `void Blockchain::createGenesisBlock()`

### What it does

Creates the hardcoded genesis block and applies it to node state.

### Why it exists

Ensures every new node starts from the same first block.

### Called by

Constructor when no stored blocks exist.

### Side effects

- inserts genesis transaction into `transactions`,
- appends block to `blocks`,
- saves block to `blocksDB`,
- updates UTXO set.

### Pitfalls

Genesis values are hardcoded; changing them breaks compatibility with existing data and any other node expecting the same origin.

## `Blockchain& Blockchain::getInstance()`

### What it does

Returns the singleton blockchain instance.

### Why it exists

Simplifies global access to node state.

### Thread safety

C++ static local initialization is thread-safe, but the object itself is not generally safe for concurrent mutation.

## `std::string Blockchain::generateBlockKey()`

### What it does

Builds a zero-padded decimal key from the current block count.

### Why it exists

Keeps LevelDB block keys lexicographically ordered by height.

### Return value

10-character numeric string.

## `Hash Blockchain::buildTarget()`

### What it does

Builds the proof-of-work target from the configured difficulty.

### Why it exists

`verifyDifficulty()` needs a threshold to compare candidate block hashes against.

### Side effects

Updates member `target`.

## `void Blockchain::loadBlocks()`

### What it does

Loads all stored blocks from LevelDB and rebuilds in-memory chain state.

### Why it exists

Restores confirmed blockchain state after restart.

### Side effects

- fills `blocks`,
- fills `transactions`,
- rebuilds `utxo`,
- fills `blocksMap`.

### Complexity

O(number of stored blocks + total transactions).

## `void Blockchain::loadPoolTransactions()`

### What it does

Loads pending transactions from the mempool DB and rebuilds input reservations.

### Why it exists

Preserves mempool state across restart.

### Side effects

- fills `transactionsPool`,
- fills `mempoolInputs`.

## `bool Blockchain::verifyDifficulty(const Hash& hash)`

### What it does

Checks whether a hash is less than or equal to the target.

### Why it exists

Implements the project’s proof-of-work acceptance condition.

## `bool Blockchain::verifyInputs(const SignedTransaction& st) const`

### What it does

Validates structural and ownership properties of transaction inputs and outputs.

### Why it exists

Ensures the transaction spends real, owned, non-duplicated, sufficient UTXOs.

### Called by

`acceptTransaction()`.

### Checks performed

- non-empty inputs and outputs,
- no duplicate inputs,
- each input exists in current UTXO set,
- each UTXO belongs to derived sender address,
- no overflow in input/output summation,
- total inputs >= total outputs.

### Complexity

O(inputs + outputs), plus hash-map lookups.

### Pitfalls

Does not independently ensure `tx.coins` equals any particular output sum pattern.

## `bool Blockchain::verifySignature(const SignedTransaction& st) const`

### What it does

Verifies detached Ed25519 signature over the transaction hash.

### Why it exists

Confirms the holder of the corresponding private key authorized the transaction.

## `void Blockchain::updateUTXO(const Transaction& transaction)`

### What it does

Applies a confirmed transaction to the current UTXO set.

### Why it exists

This is the core state-transition step for confirmed transactions.

### Side effects

- removes spent outputs,
- inserts newly created outputs.

## `std::expected<void, Blockchain::TransactionRejection> Blockchain::acceptTransaction(const SignedTransaction& st)`

### What it does

Attempts full mempool acceptance of a signed transaction.

### Why it exists

Centralizes transaction admission policy.

### Return value

- success: empty `expected`
- failure: rejection code and reason

### Side effects on success

- inserts into `transactionsPool`,
- reserves inputs in `mempoolInputs`,
- persists to `poolsDB`.

### Pitfalls

This function mutates state; callers should not treat it as a pure validation check.

## `std::expected<void, std::string> Blockchain::addTransaction(const SignedTransaction& signedTransaction)`

### What it does

Thin public wrapper around `acceptTransaction()`.

### Why it exists

Exposes a simpler public interface using a string error message.

## `Hash Blockchain::getCurrentBlockHash() const`

### What it does

Returns the hash of the current chain tip.

### Why it exists

Useful for future block construction or status inspection.

## `bool Blockchain::transactionInPool(const std::string& txHash) const`

### What it does

Checks whether a transaction hash already exists in the mempool.

### Why it exists

Prevents duplicate pending transactions.

## `std::string Blockchain::serializeBlock(const Block& block)`

### What it does

Delegates to `Block::serialize()`.

### Why it exists

Keeps block persistence code inside `Blockchain` readable.

## `Block Blockchain::deserializeBlock(const std::string& bytes)`

### What it does

Delegates to `Block` deserialization constructor.

## `bool Blockchain::verifyCoinbaseTransaction(const Transaction& tx) const`

### What it does

Checks whether a transaction is a valid coinbase-style reward transaction.

### Rules

- no inputs,
- exactly one output,
- output amount <= `MINER_REWARD`.

## `bool Blockchain::verifyBlock(const Block& block)`

### What it does

Performs basic block validity checks.

### Why it exists

Provides a block acceptance policy foundation.

### Checks

- non-empty transaction list,
- first transaction valid coinbase,
- remaining transactions already in mempool,
- correct Merkle root,
- correct previous hash linkage,
- satisfies difficulty target.

### Limitations

Does not itself append or persist the block.

## `Addr Blockchain::decodeAddressRequest(std::span<const unsigned char> payload)`

### What it does

Parses an address-only request payload.

### Why it exists

Used by address-based query handlers.

### Error handling

Throws if payload length is not exactly `AddrSize`.

## `SignedTransaction Blockchain::deserializeCreateTransactionRequest(std::span<const unsigned char> payload)`

### What it does

Parses a binary `CreateTransaction` payload into a `SignedTransaction`.

### Why it exists

Separates byte-level protocol parsing from higher-level business logic.

### Error handling

Throws on malformed counts, truncation, or trailing bytes.

### Complexity

O(inputs + outputs).

### Coroutine behavior

None. Pure parsing function.

## `Blockchain::AddressUtxos Blockchain::findAddressUtxos(const Addr& address) const`

### What it does

Scans the full UTXO set and collects entries owned by the given address.

### Why it exists

Supports `GetUTXOs` queries.

### Return value

Internal struct containing matching `Input` references and total coin sum.

### Complexity

O(total UTXO count).

### Pitfalls

Can become expensive as the chain grows because no address index exists.

## `std::vector<unsigned char> Blockchain::serializeUtxosResponse(const AddressUtxos& addressUtxos)`

### What it does

Encodes the result of an address UTXO query into response bytes.

### Why it exists

Forms the payload for `UTXOsResponse` packets.

## `std::vector<unsigned char> Blockchain::serializeTransactionResponse(bool accepted, TransactionErrorCode errorCode, std::string_view reason)`

### What it does

Builds a transaction submission response payload.

### Why it exists

Allows uniform success/rejection replies.

### Error handling

Throws if reason text is longer than `uint16_t` can encode.

## `asio::awaitable<void> Blockchain::sendPacket(...)`

### What it does

Wraps a payload into a `Packet` and asynchronously writes it to the socket.

### Why it exists

Centralizes network response framing.

### Coroutine behavior

Suspends during `asio::async_write`.

## `asio::awaitable<void> Blockchain::sendTransactionResponse(...)`

### What it does

Helper that sends a `TransactionResponse` packet.

### Why it exists

Avoids repeating payload-type and serialization details.

## `asio::awaitable<void> Blockchain::readMessage(std::shared_ptr<asio::ip::tcp::socket> socket)`

### What it does

Reads one framed packet from a socket and dispatches it.

### Why it exists

This is the main per-connection read coroutine.

### Error handling

Catches exceptions and logs a rejection.

### Coroutine behavior

Suspends during both `async_read` calls and during delegated handler execution.

### Pitfalls

Currently handles one message rather than looping indefinitely per connection.

## `void Blockchain::acceptClient()`

### What it does

Schedules asynchronous acceptance of the next client connection.

### Why it exists

Maintains a continuously listening server.

### Side effects

- creates new socket,
- recursively schedules the next accept,
- spawns `readMessage()` on success.

## `void Blockchain::setupConnection()`

### What it does

Starts accepting clients and runs the Asio event loop.

### Why it exists

This is the public start-server entry point used by `main()`.

## `asio::awaitable<void> Blockchain::handlePayload(PayloadType type, std::span<const unsigned char> payload, std::shared_ptr<asio::ip::tcp::socket> socket)`

### What it does

Dispatches a parsed payload by enum type.

### Why it exists

Separates frame parsing from per-message behavior.

### Current implemented cases

- `GetUTXOs`
- `CreateTransaction`

### Unsupported behavior

Logs an error for other message types.

## `asio::awaitable<void> Blockchain::handleGetUTXOs(...)`

### What it does

Handles an address query and returns a `UTXOsResponse`.

### Flow

- decode address,
- find matching UTXOs,
- serialize response,
- send packet.

## `asio::awaitable<void> Blockchain::handleCreateTransaction(...)`

### What it does

Implements the full network transaction submission flow.

### Why it exists

This is the main externally visible business operation in the node.

### Steps

1. parse request payload,
2. reject malformed payloads,
3. verify sender/public-key match,
4. call `acceptTransaction()`,
5. translate result into `TransactionResponse`.

### Error handling

Returns structured rejection codes to clients whenever possible.

### Coroutine behavior

Suspends while writing the response.

---

# `axis/src/core/logger.cpp`

## `Logger::log`, `Logger::debug`, `Logger::error`, `Logger::reject`

### What they do

Print prefixed log lines to standard output.

### Why they exist

Provide minimal visibility into runtime behavior.

### Side effects

Console output.

### Limitations

No log levels, sinks, timestamps, or structured metadata.
