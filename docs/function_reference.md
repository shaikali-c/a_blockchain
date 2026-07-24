# Function reference

Complete reference for every free function in the Axis codebase.

## `types.h`

### `operator==` / `operator!=` (OutPoint)

```cpp
bool operator==(const OutPoint& a, const OutPoint& b);
bool operator!=(const OutPoint& a, const OutPoint& b);
```

Compares both `txid` and `index`. Required by `std::unordered_map`.

### `std::hash<OutPoint>`

```cpp
template<> struct std::hash<OutPoint> {
    size_t operator()(const OutPoint& op) const noexcept;
};
```

Hashes all 36 bytes of the OutPoint (32 + 4) using a bit-mixing approach:
XOR of the first 8 bytes of txid plus the index.

---

## `util.h`

### `time_since_epoch`

```cpp
uint64_t time_since_epoch();
```

Returns the current Unix timestamp in seconds. Wraps
`std::chrono::system_clock::now().time_since_epoch()`.

### `hash_to_hex`

```cpp
std::string hash_to_hex(const Hash& h);
```

Converts a 32-byte hash to a 64-character hex string (lowercase, no `0x`
prefix). Used for debug output only — never in serialization or UTXO keys.

---

## `crypto.h`

### `blake2b`

```cpp
Hash blake2b(std::span<const uint8_t> data);
```

Computes the Blake2b-256 hash of arbitrary data. Used for txids, block
hashes, Merkle tree nodes, and address derivation.

### `generate_keypair`

```cpp
void generate_keypair(PublicKey& pk, PrivateKey& sk);
```

Generates a new Ed25519 key pair. Calls `crypto_sign_keypair`.

- `pk` — output, 32 bytes
- `sk` — output, 64 bytes (libsodium format: 32 seed + 32 cached public key)

### `sign_msg`

```cpp
Signature sign_msg(const PrivateKey& sk, const Hash& msg);
```

Signs a message hash with a private key using Ed25519.

- `sk` — Ed25519 private key (64 bytes)
- `msg` — the 32-byte hash to sign (typically a txid)
- Returns: 64-byte detached signature

### `verify_sig`

```cpp
bool verify_sig(const PublicKey& pk, const Hash& msg, const Signature& sig);
```

Verifies an Ed25519 signature.

- Returns: `true` if the signature is valid, `false` otherwise
- Uses `crypto_sign_verify_detached`

### `derive_address`

```cpp
Address derive_address(const PublicKey& pk);
```

Derives a 20-byte address from a 32-byte public key.

- Uses `crypto_generichash` with output length = 20 bytes
- Used to check UTXO ownership: `derive_address(pubkey) == utxo.recipient`

---

## `tx.cpp`

### `Transaction::serialize`

```cpp
std::vector<uint8_t> Transaction::serialize() const;
```

Serializes the transaction to bytes. Format:

```
[txid (32)] [input_count (4 LE)] [inputs...] [output_count (4 LE)] [outputs...] [timestamp (8 LE)]
```

### `Transaction::Transaction(span)`

```cpp
Transaction::Transaction(std::span<const uint8_t> data);
```

Deserializes a transaction from bytes. Recomputes the txid from parsed
data (in case the stored txid is stale).

### `SignedTransaction::serialize`

```cpp
std::vector<uint8_t> SignedTransaction::serialize() const;
```

Serializes a signed transaction. Format:

```
[pubkey (32)] [timestamp (8 LE)] [input_count (4 LE)] [inputs...] [output_count (4 LE)] [outputs...] [signature (64)]
```

Note: the txid is NOT included. The receiver must recompute it.

### `SignedTransaction::SignedTransaction(span)`

```cpp
SignedTransaction::SignedTransaction(std::span<const uint8_t> data);
```

Deserializes a signed transaction. Parses pubkey, timestamp, inputs,
outputs, and signature. The Transaction is constructed from the parsed
components (which computes its txid).

---

## `block.cpp`

### `Block::Block(prev_hash, txs, timestamp, nonce)`

```cpp
Block::Block(Hash prev_hash, std::vector<Transaction> txs,
             uint64_t timestamp, uint32_t nonce);
```

Constructs a block with the given header fields. Automatically:

1. Sorts/stores transactions
2. Computes the Merkle root from all txids

### `Block::hash`

```cpp
Hash Block::hash() const;
```

Computes the block hash: `blake2b(header_bytes)`. Header bytes:

```
[prev_hash (32)] [merkle_root (32)] [timestamp (8)] [nonce (4)] [version (4)]
```

Total: 80 bytes input → 32 bytes output.

### `Block::verifyDifficulty`

```cpp
bool Block::verifyDifficulty() const;
```

Checks that the first 3 bytes of the block hash are `0x00`. This is the
Proof of Work check for difficulty = 3.

```cpp
// Equivalent:
bool ok = h[0] == 0x00 && h[1] == 0x00 && h[2] == 0x00;
```

### `Block::serialize`

```cpp
std::vector<uint8_t> Block::serialize() const;
```

Serializes the full block (header + transactions):

```
[prev_hash (32)] [merkle_root (32)] [timestamp (8)] [nonce (4)] [version (4)]
[tx_count (4 LE)] [Transaction 0...] [Transaction 1...]
```

### `Block::Block(span)`

```cpp
Block::Block(std::span<const uint8_t> data);
```

Deserializes a block from bytes. Reconstructs the header and all
transactions.

### `compute_block_merkle_root`

```cpp
Hash compute_block_merkle_root(const std::vector<Transaction>& txs);
```

Builds a Merkle tree from the transaction hashes and returns the root.

Algorithm:
1. Collect all txids
2. While more than 1 hash remains:
   - If odd count, duplicate the last element
   - Pair and hash each pair
3. Return the single remaining hash (or all-zeros for empty tx list)

### `buildTarget`

```cpp
Hash buildTarget(uint8_t difficulty);
```

Builds a 32-byte target value for Proof of Work. The first `difficulty`
bytes are `0x00`, the rest are `0xFF`. For difficulty = 3:

```
target = 0x000000FFFF...FF
```

A valid block hash must be ≤ this target (when both are interpreted as
big-endian 256-bit integers).

### `hash_to_uint256`

```cpp
uint256_t hash_to_uint256(const Hash& h);
```

Converts a 32-byte hash to a 256-bit unsigned integer (from Boost's
`boost::multiprecision::uint256_t`) via big-endian byte order.

### `hash_to_hex` (block.cpp overload)

```cpp
std::string hash_to_hex(const std::array<uint8_t, 32>& h);
```

Same as `util.h`'s version. Block.cpp keeps its own copy to avoid a
cross-layer dependency from block → util.

---

## `chain.cpp`

### `Chain::Chain`

```cpp
Chain::Chain();
```

Constructor. In order:

1. Opens LevelDB databases `blocks/` and `pool/`
2. Calls `load_blocks()` — reads all blocks, reconstructs UTXO set
3. Calls `load_pool()` — reloads pending transactions into mempool
4. If no blocks found, calls `create_genesis()`

### `Chain::create_genesis`

```cpp
void Chain::create_genesis();
```

Creates the genesis block (height 0):

- `prev_hash` = all zeros
- One coinbase transaction: output to `GENESIS_ADDR` with value
  `15 * UNITS` = 15,000,000
- Timestamp: `1781545365` (a fixed historical value)
- Nonce: `31496` (pre-mined: this nonce satisfies difficulty 3)
- Stores the block to LevelDB

### `Chain::add_tx`

```cpp
TxError Chain::add_tx(const SignedTransaction& stx);
```

Validates and adds a transaction to the mempool.

**Validation order:**
1. Output sum > 0
2. Transaction has inputs
3. All input UTXOs exist
4. All input UTXOs belong to the signer's address
5. Input sum >= output sum
6. Signature is valid
7. Not already in mempool
8. No input already spent in mempool

**On success:** stores to `pool_` and `pool_db_`.

### `Chain::add_block`

```cpp
BlockError Chain::add_block(Block& blk);
```

Validates and applies a mined block.

**Validation order:**
1. Height matches expected next height
2. `prev_hash` matches chain tip
3. Block hash matches recomputed hash
4. Proof of Work is valid (hash <= target)
5. Timestamp within acceptable window
6. All non-coinbase transactions have valid signatures
7. All non-coinbase inputs exist in UTXO set
8. Coinbase is first transaction and has no inputs

**On success:**
1. Applies each transaction to the UTXO set
2. Removes confirmed transactions from mempool
3. Stores block to LevelDB
4. Advances chain tip

### `Chain::apply_tx`

```cpp
void Chain::apply_tx(Transaction& tx);
```

Internal method. Applies a confirmed transaction to the UTXO set:

1. Erases all input OutPoints from `utxo_`
2. Inserts all outputs as new UTXOs keyed by `(tx.txid(), index)`

Does NOT validate (validation happens before calling this).

### `Chain::get_utxos`

```cpp
std::pair<std::vector<OutPoint>, uint64_t>
Chain::get_utxos(const Address& addr) const;
```

Returns all UTXOs belonging to `addr` plus the total sum.

### `Chain::get_tx`

```cpp
std::optional<Transaction> Chain::get_tx(const Hash& txid) const;
```

Searches all blocks for a transaction with the given txid. Returns
`std::nullopt` if not found.

### `Chain::get_mempool_tx`

```cpp
std::optional<Transaction> Chain::get_mempool_tx(const Hash& txid) const;
```

Looks up a transaction in the mempool by txid.

### `Chain::get_block_range`

```cpp
std::vector<Block> Chain::get_block_range(uint32_t start, uint32_t end) const;
```

Returns blocks from heights `start` to `end` inclusive.

### `Chain::load_blocks`

```cpp
void Chain::load_blocks();
```

Reads all blocks from LevelDB using an iterator. Each block is
deserialized and its transactions are applied to the UTXO set. Skips
sentinel keys (those not exactly 8 bytes).

### `Chain::load_pool`

```cpp
void Chain::load_pool();
```

Reads all pending transactions from LevelDB's pool database into memory.

### `Chain::store_block`

```cpp
void Chain::store_block(Block& blk);
```

Writes a block to LevelDB: block data at key `height_key(height_)`, tip
hash at sentinel `-1`, and height at sentinel `-2`.

### `open_db` (chain.cpp internal)

```cpp
static std::unique_ptr<leveldb::DB> open_db(const std::string& name);
```

Opens or creates a LevelDB database in `./axis_data/<name>/`.

---

## `net.cpp`

### `Server::Server`

```cpp
Server::Server(Chain& chain);
```

Creates the server with a reference to the chain. Does NOT start listening
— call `start()` to begin.

### `Server::set_port`

```cpp
void Server::set_port(uint16_t port);
```

Sets the listening port. Default is 8080.

### `Server::start`

```cpp
void Server::start(uint16_t port);
```

Binds to the port, starts accepting connections, and runs the Asio event
loop. Blocks the calling thread.

### `Server::do_accept`

```cpp
asio::awaitable<void> Server::do_accept();
```

Accept loop: waits for a new TCP connection, spawns a `session` coroutine
for it, and repeats.

### `Server::session`

```cpp
asio::awaitable<void> Server::session(asio::ip::tcp::socket sock);
```

Per-connection coroutine. Reads packets in a loop:
1. Read 9-byte header (magic + type + payload_length)
2. Verify magic == `0xDEADBEEF`
3. Read payload
4. Dispatch to the appropriate handler
5. Exit on read error (connection closed)

### `async_read` (net.cpp free function)

```cpp
asio::awaitable<std::vector<uint8_t>> async_read(
    asio::ip::tcp::socket& sock, size_t n);
```

Reads exactly `n` bytes from the socket. Throws on disconnect.

### `async_write` (net.cpp free function)

```cpp
asio::awaitable<void> async_write(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> data);
```

Writes all bytes to the socket.

### `send_payload` (net.cpp free function)

```cpp
asio::awaitable<void> send_payload(
    asio::ip::tcp::socket& sock, MsgType type,
    const std::vector<uint8_t>& payload);
```

Writes a complete packet: 9-byte header + payload bytes.

### `send_txresponse` (net.cpp free function)

```cpp
asio::awaitable<void> send_txresponse(
    asio::ip::tcp::socket& sock, bool accepted,
    TxError err, const std::string& reason);
```

Writes a `TransactionResponse` packet.

### `on_get_utxos`

```cpp
asio::awaitable<void> Server::on_get_utxos(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
```

Handler for `GetUTXOs (3)`:
1. Read 20-byte address from payload
2. Call `chain_.get_utxos(addr)`
3. Serialize and send `SendUTXOs (4)` response

### `on_create_tx`

```cpp
asio::awaitable<void> Server::on_create_tx(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
```

Handler for `CreateTransaction (12)`:
1. Deserialize `SignedTransaction` from payload
2. Call `chain_.add_tx(stx)`
3. Send `TransactionResponse (13)` with result

### `on_get_tx`

```cpp
asio::awaitable<void> Server::on_get_tx(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
```

Handler for `GetTx (5)`:
1. Read 32-byte txid from payload
2. Call `chain_.get_tx(txid)`
3. Send `SendTx (6)` with serialized transaction (or empty)

### `on_get_mempool_tx`

```cpp
asio::awaitable<void> Server::on_get_mempool_tx(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
```

Handler for `GetMempoolTx (11)`:
1. Read 32-byte txid from payload
2. Call `chain_.get_mempool_tx(txid)`
3. Send `SendTx (6)` with serialized transaction (or empty)

### `on_get_block_range`

```cpp
asio::awaitable<void> Server::on_get_block_range(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
```

Handler for `GetBlockRange (9)`:
1. Read `start_range` (uint32 LE) and `end_range` (uint32 LE)
2. Call `chain_.get_block_range(start, end)`
3. Serialize and send `SendBlockRange (10)` response

---

## `main.cpp`

### `main`

```cpp
int main(int argc, char** argv);
```

Entry point:
1. Check for `--help` flag
2. Create `Chain` (loads from disk or creates genesis)
3. Create `Server` with reference to chain
4. Start server on port 8080
5. Print status message
6. Run event loop (blocks until Ctrl+C)
