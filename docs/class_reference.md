# Class reference

Complete reference for every class in the Axis codebase.

---

## Hash

**File:** `include/axis/types.h`

A 32-byte (256-bit) cryptographic hash. Used for transaction IDs (txid),
block hashes, Merkle roots, and OutPoint references.

```cpp
using Hash = std::array<uint8_t, 32>;
```

### Notable behavior

- Default-constructs to all zeros (the "null hash")
- Supports comparison operators (`==`, `!=`)
- Can be used as an `unordered_map` key with `HashHasher`

---

## Address

**File:** `include/axis/types.h`

A 20-byte (160-bit) identifier for a user. Derived by hashing the public
key with Blake2b.

```cpp
using Address = std::array<uint8_t, 20>;
```

---

## OutPoint

**File:** `include/axis/types.h`

A reference to a specific output of a previous transaction. Used as
transaction inputs to identify which UTXO is being spent.

```cpp
struct OutPoint {
    Hash txid;             // 32 bytes — the transaction containing the output
    uint32_t index;        // 4 bytes — which output in that transaction
};
```

### Equality

Two OutPoints are equal if both `txid` and `index` match. Used for UTXO
set lookups and mempool spent tracking.

### Hash support

`OutPoint` can be used as an `unordered_map` key (a `std::hash`
specialization is provided in `types.h`).

---

## TxOutput

**File:** `include/axis/types.h`

A destination for coins in a transaction.

```cpp
struct TxOutput {
    Address recipient;     // 20 bytes — who receives these coins
    uint64_t amount;       // 8 bytes — how many coins
};
```

---

## Writer

**File:** `include/axis/types.h`

A serialization helper that appends typed data to a byte buffer.

```cpp
struct Writer {
    std::vector<uint8_t> buf;

    void put_u8(uint8_t v);
    void put_u16(uint16_t v);      // little-endian
    void put_u32(uint32_t v);      // little-endian
    void put_u64(uint64_t v);      // little-endian
    void put_u32_be(uint32_t v);   // big-endian
    void put_varint(uint64_t v);
    void put_hash(const Hash& h);
    void put_addr(const Address& a);
    void put_pk(const PublicKey& pk);
    void put_sig(const Signature& sig);
    void put_bytes(std::span<const uint8_t> data);
};
```

### Usage pattern

```cpp
Writer w;
w.put_u32(count);
w.put_hash(some_hash);
w.put_u64(timestamp);
// w.buf now contains the serialized bytes
```

---

## Reader

**File:** `include/axis/types.h`

A deserialization helper that extracts typed data from a byte buffer.

```cpp
struct Reader {
    std::span<const uint8_t> buf;
    size_t offset = 0;

    uint8_t     take_u8();
    uint16_t    take_u16();
    uint32_t    take_u32();
    uint64_t    take_u64();
    uint32_t    take_u32_be();
    uint64_t    take_varint();
    Hash        take_hash();
    Address     take_addr();
    PublicKey   take_pk();
    Signature   take_sig();
    std::span<const uint8_t> take_bytes(size_t n);
};
```

### Usage pattern

```cpp
Reader r{buffer};
auto count = r.take_u32();
auto hash = r.take_hash();
auto ts = r.take_u64();
```

Reader advances `offset` after each read. It does not bounds-check — the
caller must ensure sufficient data.

---

## HashHasher

**File:** `include/axis/types.h`

A custom hash functor for using `Hash` as an `unordered_map` key.

```cpp
struct HashHasher {
    size_t operator()(const Hash& h) const noexcept {
        size_t r = 0;
        std::memcpy(&r, h.data(), sizeof(r));
        return r;
    }
};
```

Uses the first 8 bytes of the hash as the hash value. Since hashes are
already uniformly distributed, this is a good hash function for the map.

---

## Transaction

**File:** `include/axis/tx.h`

A blockchain transaction that spends UTXOs and creates new ones.

```cpp
class Transaction {
    std::vector<OutPoint> inputs_;
    std::vector<TxOutput> outputs_;
    uint64_t timestamp_;
    Hash txid_;  // memoized, computed in constructor

public:
    Transaction(std::vector<OutPoint> inputs,
                std::vector<TxOutput> outputs,
                uint64_t timestamp);

    const Hash& txid() const { return txid_; }
    bool is_coinbase() const { return inputs_.empty(); }

    // Serialization
    std::vector<uint8_t> serialize() const;
    explicit Transaction(std::span<const uint8_t> data);

    // Accessors (friends of Chain for validation)
    friend class Chain;
};
```

### Private fields (accessed by Chain through friendship)

```cpp
const std::vector<OutPoint>& get_inputs() const { return inputs_; }
const std::vector<TxOutput>& get_outputs() const { return outputs_; }
uint64_t get_timestamp() const { return timestamp_; }
```

---

## SignedTransaction

**File:** `include/axis/tx.h`

A transaction bundled with the public key and signature needed to verify
it.

```cpp
struct SignedTransaction {
    Transaction tx;
    PublicKey pubkey;    // 32 bytes — the signer's public key
    Signature sig;       // 64 bytes — Ed25519 signature
};
```

### Serialization

The serialized form does not include the txid (it is recomputed from the
data). The public key is at the start so the receiver can derive the
address before parsing the inputs/outputs.

---

## Block

**File:** `include/axis/block.h`

A block in the blockchain, containing a proof-of-work header and a list of
transactions.

```cpp
class Block {
    struct Header {
        Hash prev_hash;       // 32 bytes
        Hash merkle_root;     // 32 bytes
        uint64_t timestamp;   // 8 bytes
        uint32_t nonce;       // 4 bytes
        uint32_t version;     // 4 bytes
    };
    Header header_;
    std::vector<Transaction> transactions_;

public:
    Block(Hash prev_hash, std::vector<Transaction> txs,
          uint64_t timestamp, uint32_t nonce);

    Hash hash() const;  // blake2b of the 80-byte header
    bool verifyDifficulty() const;

    // Accessors
    const std::vector<Transaction>& transactions() const;
    const Hash& prev_hash() const;
    uint64_t timestamp() const;

    // Serialization
    std::vector<uint8_t> serialize() const;
    explicit Block(std::span<const uint8_t> data);
};
```

### Constructed automatically

The constructor computes the Merkle root from the transactions and stores
it in `header_.merkle_root`.

---

## Chain

**File:** `include/axis/chain.h`

The core blockchain node. Owns blocks, UTXO set, mempool, and databases.

```cpp
class Chain {
    std::vector<Block> blocks_;    // all blocks, by height
    Hash tip_hash_;
    uint64_t height_ = 0;

    std::unordered_map<OutPoint, TxOutput> utxo_;
    std::unordered_map<Hash, Transaction, HashHasher> pool_;
    std::unordered_map<OutPoint, OutPoint> pool_spent_;

    std::unique_ptr<leveldb::DB> blocks_db_;
    std::unique_ptr<leveldb::DB> pool_db_;

    static constexpr Hash GENESIS_ADDR = {{
        0xf4, 0x5a, 0x20, 0xe0, 0x43, 0xb0, 0x1f, 0x65,
        0x63, 0x8a, 0x46, 0x83, 0x1c, 0xe7, 0x9b, 0x8f,
        0xec, 0x3f, 0x67, 0x37
    }};
    static constexpr uint64_t UNITS = 1000000;

public:
    Chain();
    TxError add_tx(const SignedTransaction& stx);
    BlockError add_block(Block& blk);
    std::pair<std::vector<OutPoint>, uint64_t>
        get_utxos(const Address& addr) const;
    std::optional<Transaction> get_tx(const Hash& txid) const;
    std::optional<Transaction> get_mempool_tx(const Hash& txid) const;
    std::vector<Block> get_block_range(uint32_t start, uint32_t end) const;

    const Hash& get_tip_hash() const;
    uint64_t get_height() const;
    void set_difficulty(uint8_t d);

private:
    void create_genesis();
    void load_blocks();
    void load_pool();
    void apply_tx(Transaction& tx);
    void store_block(Block& blk);
};
```

### Key methods

| Method | Description |
|--------|-------------|
| `Chain()` | Constructor. Opens LevelDB databases, loads blocks, rebuilds UTXO set, reloads mempool. Creates genesis if no blocks exist. |
| `add_tx` | Validate a signed transaction and add it to the mempool. Returns `TxError::None` on success. |
| `add_block` | Validate a mined block and apply it to the chain. Returns `BlockError::None` on success. |
| `get_utxos` | Return all UTXOs belonging to an address, plus the total sum. |
| `get_tx` | Look up a confirmed transaction by txid. |
| `get_mempool_tx` | Look up a pending transaction by txid. |
| `get_block_range` | Return blocks [start, end) inclusive. |
| `set_difficulty` | Set the mining difficulty (1 byte, default 3). |

---

## Server

**File:** `include/axis/net.h`

The TCP server that accepts connections and handles messages.

```cpp
class Server {
    asio::io_context ctx_;
    asio::ip::tcp::acceptor acceptor_;
    Chain& chain_;

public:
    explicit Server(Chain& chain);
    void set_port(uint16_t port);
    void start(uint16_t port);
};
```

### Handler coroutines (private)

```cpp
asio::awaitable<void> do_accept();
asio::awaitable<void> session(asio::ip::tcp::socket sock);
asio::awaitable<void> on_get_utxos(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
asio::awaitable<void> on_create_tx(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
asio::awaitable<void> on_get_tx(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
asio::awaitable<void> on_get_mempool_tx(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
asio::awaitable<void> on_get_block_range(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> payload);
```

### Free functions (helpers in net.cpp)

```cpp
asio::awaitable<std::vector<uint8_t>> async_read(
    asio::ip::tcp::socket& sock, size_t n);
asio::awaitable<void> async_write(
    asio::ip::tcp::socket& sock, std::span<const uint8_t> data);
asio::awaitable<void> send_payload(
    asio::ip::tcp::socket& sock, MsgType type,
    const std::vector<uint8_t>& payload);
asio::awaitable<void> send_txresponse(
    asio::ip::tcp::socket& sock, bool accepted,
    TxError err, const std::string& reason);
```

---

## TxError (enum)

**File:** `include/axis/tx.h`

Error codes for transaction validation.

```cpp
enum class TxError : uint8_t {
    None = 0,
    InvalidPayload,
    ZeroAmount,
    BadOwnership,
    BadSignature,
    Duplicate,
    InputSpent,
};
```

---

## BlockError (enum)

**File:** `include/axis/block.h`

Error codes for block validation.

```cpp
enum class BlockError : uint8_t {
    None = 0,
    InvalidHeight,
    BadPreviousHash,
    InvalidBlockHash,
    HighHash,
    TimeTooFar,
    TimeTooOld,
    BadSignature,
    MissingInputs,
    InvalidPayload,
};
```

---

## MsgType (enum)

**File:** `include/axis/net.h`

Network message types for the TCP protocol.

```cpp
enum class MsgType : uint8_t {
    FullBlocks     = 1,
    GetUTXOs       = 3,
    SendUTXOs      = 4,
    GetTx          = 5,
    SendTx         = 6,
    GetBlock       = 7,    // not handled
    SendBlock      = 8,    // not handled
    GetBlockRange  = 9,    // not handled
    SendBlockRange = 10,
    GetMempoolTx   = 11,
    CreateTransaction = 12,
    TransactionResponse = 13,
};
```

---

## Key/PK types

**File:** `include/axis/crypto.h`

```cpp
using PublicKey = std::array<uint8_t, 32>;
using PrivateKey = std::array<uint8_t, 64>;
using Signature = std::array<uint8_t, 64>;
```

- `PublicKey` — Ed25519 public key (32 bytes)
- `PrivateKey` — Ed25519 private key (64 bytes: 32 seed + 32 cached public
  key, as returned by libsodium)
- `Signature` — Ed25519 detached signature (64 bytes)
