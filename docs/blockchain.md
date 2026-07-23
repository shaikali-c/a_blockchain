# Blockchain Concepts Explained Through Axis

This document explains blockchain ideas from zero and ties each one directly to the Axis codebase.

## 1. What is a blockchain?

A blockchain is a growing sequence of blocks.

Each block contains a batch of transactions plus a reference to the previous block. Because each block points backward, the blocks form a chain.

In Axis, the chain is stored in memory as:

- `std::vector<Block> blocks`

and persisted to LevelDB in `blocks/`.

## 2. Why do blocks exist?

Without blocks, transactions would just be a loose pile of events. Blocks give the system:

- ordering,
- grouping,
- a history structure,
- a place to attach proof-of-work.

In Axis, a `Block` contains:

- `previous_hash`: links to the prior block,
- `hash`: block identifier / proof target result,
- `timestamp`,
- `nonce`,
- `merkleRoot`,
- `transactions`.

## 3. What is a transaction?

A transaction describes movement of value.

In UTXO-style systems, a transaction does two things:

1. spends previous outputs,
2. creates new outputs.

In Axis, `Transaction` stores:

- `inputs`: references to earlier outputs being spent,
- `outputs`: newly created outputs,
- `sender`,
- `receiver`,
- `coins`,
- `timestamp`,
- `transaction_hash`.

## 4. What is a UTXO?

`UTXO` stands for **Unspent Transaction Output**.

Plain-English meaning: a chunk of value created by an earlier transaction that has not been spent yet.

Think of it like a coin receipt. If the receipt has not been used yet, it can be used as input to a new payment.

In Axis, a `UTXO` contains:

- `owner`: the address allowed to spend it,
- `coins`: the amount.

The node stores the current spendable set in:

- `std::unordered_map<std::string, UTXO> utxo`

The key is a string like:

```text
<transaction_hash_hex>:<output_index>
```

Example:

```text
abcd1234...ff:0
```

This means “output 0 of transaction `abcd1234...ff`”.

## 5. Why are hashes used?

A hash is a fixed-size fingerprint of data.

Hashes are useful because they let the system:

- identify transactions,
- identify blocks,
- commit to large data compactly,
- compare values quickly,
- make tampering visible.

In Axis:

- transaction hashes are computed from transaction content,
- block headers store hashes,
- Merkle roots are built from transaction hashes,
- addresses are also derived using hashing.

Axis uses libsodium’s generic hash function via `crypto_generichash`.

## 6. What is proof-of-work here?

Proof-of-work usually means a block hash must be smaller than a target.

Axis uses a very simple target model:

- `difficulty` is a byte count,
- `buildTarget()` fills the hash with `0xff`,
- then sets the first `difficulty` bytes to `0x00`,
- `verifyDifficulty()` checks `hash <= target`.

That means the block hash must start with enough low bytes to be numerically small enough.

### Important limitation

The repository does **not** currently include a mining loop that searches for valid nonces. The validation rule exists, but automatic mining is not implemented in the visible code.

## 7. Why are blocks chained together?

Every block stores the previous block’s hash.

That means changing an earlier block would break the link from the next block onward.

In Axis, block verification checks:

- `block.blockHeader.previous_hash == blocks.back().blockHeader.hash`

That ensures a candidate block extends the current tip.

## 8. What is a Merkle tree?

A Merkle tree is a way to summarize many transaction hashes into one hash called the **Merkle root**.

Why do this?

- A block can store one root instead of repeating full transaction contents in the header.
- Any change in a transaction changes the root.

Axis computes Merkle roots in `Cryptography::computeMerkleRoot()`.

### How Axis does it

1. Start with a list of transaction hashes.
2. If the count is odd, duplicate the last one.
3. Pair hashes two by two.
4. Concatenate each pair.
5. Hash the concatenation.
6. Repeat until one hash remains.

## 9. What are public and private keys?

A private key is a secret used to authorize spending.

A public key is the shareable counterpart others can use to verify signatures.

In Axis:

- `PublicKey` is a fixed-size byte array,
- `SecretKey` type exists in headers,
- signature verification is done with libsodium Ed25519.

### Important limitation

The current repository verifies signatures, but does not provide a full wallet or key generation flow.

## 10. What is a digital signature?

A digital signature proves that someone holding the private key approved a message.

In Axis, the signed message is the transaction hash.

The node checks signatures in:

- `Blockchain::verifySignature()`

It calls:

- `crypto_sign_verify_detached(...)`

If verification fails, the transaction is rejected.

## 11. How do addresses work here?

An address is a shorter identifier derived from a public key.

Axis computes an address by hashing the public key and keeping 20 bytes:

- `Addr computeAddress(const PublicKey& pk)`

That means:

- the sender address is expected to be derivable from the submitted public key,
- `handleCreateTransaction()` explicitly checks that relationship.

This prevents a client from claiming “I am sender X” while submitting a different public key.

## 12. How validation works in Axis

When a transaction is submitted, the node validates several things.

### Step 1: payload must parse correctly

The binary packet must match the expected layout.

### Step 2: sender must match public key

`computeAddress(publicKey)` must equal `transaction.sender`.

### Step 3: transaction amount must be non-zero

`tx.coins == 0` is rejected.

### Step 4: inputs must be valid

The node checks that:

- at least one input exists,
- at least one output exists,
- each referenced UTXO exists,
- each referenced UTXO belongs to the derived sender address,
- no input appears twice inside the same transaction,
- summed inputs cover summed outputs,
- arithmetic does not overflow.

### Step 5: signature must verify

The signature must match the transaction hash and public key.

### Step 6: mempool rules must pass

The node rejects:

- duplicate mempool transaction hashes,
- inputs already reserved by another pending transaction.

## 13. What is the mempool?

The mempool is the waiting area for valid transactions that are not yet inside a block.

Axis stores it in two forms:

- in memory: `transactionsPool`,
- on disk: LevelDB in `pool/`.

The extra `mempoolInputs` map reserves referenced inputs so two pending transactions cannot spend the same UTXO at the same time.

## 14. What is the genesis block?

The genesis block is the first block of the chain.

All nodes need a shared starting point.

In Axis:

- the genesis block is hardcoded,
- its block hash, Merkle root, timestamp, nonce, and reward output are fixed,
- it is created only if the chain database is empty.

This design makes node bootstrapping easy, because every fresh node starts from the same origin.

## 15. How these ideas map directly to code

| Concept | Axis implementation |
|---|---|
| Blockchain | `std::vector<Block> blocks` |
| Block | `Block` and `BlockHeader` |
| Transaction | `Transaction` |
| Spendable coin | `UTXO` |
| Reference to spendable coin | `Input` |
| Mempool | `transactionsPool` + `mempoolInputs` |
| Block storage | LevelDB `blocks/` |
| Mempool storage | LevelDB `pool/` |
| Address derivation | `computeAddress()` |
| Signature verification | `verifySignature()` |
| Merkle root | `Cryptography::computeMerkleRoot()` |
| Difficulty target | `buildTarget()` / `verifyDifficulty()` |

## 16. Beginner summary

If you remember only one thing, remember this:

- A transaction spends older outputs and creates new outputs.
- The UTXO set is the list of outputs that are still spendable.
- A block groups transactions and links to the previous block.
- Hashes and signatures let the node detect tampering and verify authorization.
- Axis is a small educational implementation of those ideas.
