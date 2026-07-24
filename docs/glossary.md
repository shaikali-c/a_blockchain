# Glossary

Every technical term used in Axis, explained in plain English.

---

**Address** — A 20-byte identifier that tells who owns a UTXO. Derived by
hashing the public key with Blake2b. Like a bank account number, but
generated from a cryptographic key. Example:
`f45a20e043b01f65638a46831ce79b8fec3f6737`

**Asio** — A C++ library for network programming. Axis uses Asio's
coroutines (`co_await`, `co_spawn`) to handle TCP connections.

**Blake2b** — A cryptographic hash function. Faster than SHA-3, as secure
as SHA-3. Axis uses it for everything: transaction hashes, Merkle trees,
address derivation.

**Block** — A batch of transactions grouped together and linked to the
previous block via its hash. The fundamental unit of the blockchain.

**Block header** — The part of a block that identifies it: previous hash,
Merkle root, timestamp, nonce, version. The block's own hash is computed
from the header.

**Blockchain** — A tamper-evident, growing list of blocks linked by
cryptographic hashes.

**Coinbase transaction** — The first transaction in a block. It has no
inputs and creates new coins as a mining reward. Also called the "generation
transaction."

**Coroutine** — A function that can suspend execution and resume later.
In Axis, network handlers use `co_await` to wait for data without blocking
the thread.

**Difficulty** — A measure of how hard it is to mine a valid block. In Axis,
difficulty = 3 means the block hash must start with 3 zero bytes.

**Double-spending** — Attempting to spend the same UTXO twice. The mempool
tracks spent UTXOs to prevent this within pending transactions.

**Ed25519** — A modern elliptic-curve signature scheme. Fast, secure, and
produces compact signatures (64 bytes) and keys (32 bytes public, 64 bytes
private).

**Genesis block** — The first block in the blockchain. It has `prev_hash`
= all zeros. Created automatically when the node runs for the first time.

**Hash** — A fixed-length fingerprint of arbitrary data. 32 bytes in Axis.
Deterministic, one-way, and collision-resistant.

**LevelDB** — An embedded key-value database from Google. Axis uses it for
persistent storage of blocks and the mempool.

**Mempool** — The set of pending (unconfirmed) transactions waiting to be
included in a block. Short for "memory pool."

**Merkle root** — A single hash that fingerprints all transactions in a
block. Built by pairing and hashing transaction hashes in a tree structure.

**Merkle tree** — A binary tree of hashes where each leaf is a transaction
hash and each internal node is the hash of its two children.

**Nonce** — A number that miners change to produce different block hashes.
Short for "number used once." In Axis, the genesis nonce is 31496.

**OutPoint** — A reference to a specific output of a previous transaction.
Consists of a txid (32 bytes) and an output index (4 bytes). What every
transaction input contains.

**Peer** — Another node on the network. Axis currently runs standalone and
does not synchronize with peers.

**Private key** — A secret 64-byte value used to sign transactions. Must
never be shared. From it, the public key is derived.

**Proof of Work (PoW)** — The consensus mechanism that requires finding a
block hash below a target. Makes tampering expensive because changing any
block requires re-mining every subsequent block.

**Public key** — A 32-byte value derived from the private key. Shared with
others. Others use it to verify your signatures and derive your address.

**RAII** — Resource Acquisition Is Initialization. A C++ idiom where
resources (memory, database handles, sockets) are acquired in constructors
and released in destructors. Axis uses RAII for LevelDB handles and sockets.

**Serialization** — Converting in-memory data structures to bytes (for
storage or network transmission) and back. Axis uses binary serialization
with `Writer` and `Reader`.

**Signature** — A 64-byte proof that the holder of a private key authorized
a specific transaction. Created by signing the transaction hash.

**Transaction (tx)** — A transfer of value. Spends existing UTXOs (inputs)
and creates new UTXOs (outputs). Identified by its txid.

**Transaction ID (txid)** — The 32-byte hash of a transaction. Used to
reference transactions in inputs and blocks.

**TxInput** — Not a separate class; transactions store `vector<OutPoint>`
for inputs. Each OutPoint references a specific UTXO to spend.

**TxOutput** — The destination of coins in a transaction. Contains a
recipient address (20 bytes) and an amount (8 bytes).

**UTXO** — Unspent Transaction Output. An output from a transaction that
has not yet been spent by any other transaction. The fundamental unit of
value in the UTXO model.

**UTXO set** — The complete collection of all unspent transaction outputs.
The node maintains this in memory. An address's balance is the sum of UTXOs
that list that address as their recipient.

**Writer / Reader** — Serialization helpers in `types.h`. `Writer`
appends typed data to a byte buffer; `Reader` extracts typed data from a
byte buffer. Used for all serialization and deserialization.
