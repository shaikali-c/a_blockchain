# Cryptography

This document explains the cryptographic ideas used in Axis without assuming prior knowledge.

## 1. What cryptography is used for in this project

Axis uses cryptography for three main jobs:

1. hashing data,
2. verifying transaction signatures,
3. deriving addresses from public keys.

It does **not** currently implement advanced cryptography like zero-knowledge proofs, script systems, or confidential transactions.

## 2. Hashing

A hash function turns input data of any size into a fixed-size output.

### Why hashes are useful

- they detect changes,
- they provide compact identifiers,
- they let blocks commit to many transactions,
- they support proof-of-work comparisons.

### Hashes in Axis

The project uses libsodium’s generic hash function:

- `crypto_generichash`

The main hash-related types are:

- `Hash = std::array<unsigned char, crypto_generichash_BYTES>`

That means hashes are fixed-size byte arrays.

## 3. Transaction hashing

Every transaction gets a `transaction_hash`.

### What is hashed

Axis hashes the serialized logical content of a transaction:

- sender,
- receiver,
- all inputs,
- all outputs,
- coin amount,
- timestamp.

### Why this exists

The hash gives the transaction a stable identity tied to its contents.

If any of those fields change, the hash changes.

### Where this happens

- `Transaction::computeTransactionHash()`
- `Transaction::computeTransactionHash(uint64_t)`

## 4. Merkle root hashing

A block needs a compact commitment to all included transactions.

Axis computes a Merkle root from the list of transaction hashes.

### Why this matters

Instead of storing “proof of all transactions” in the header directly, one root hash represents the full set.

### Where this happens

- `Cryptography::computeMerkleRoot()`

### Axis behavior with odd numbers of hashes

If a level has an odd count, Axis duplicates the last hash before combining pairs.

This is a common Merkle-tree simplification.

## 5. Digital signatures

A digital signature is a cryptographic proof that the sender approved some message.

In Axis, the message is the transaction hash.

### Important idea

The node does **not** need the private key to verify a signature.
It only needs:

- the public key,
- the message,
- the signature.

### Signature system used

Axis uses Ed25519 verification via libsodium:

- `crypto_sign_verify_detached`

### Where verification happens

- `Blockchain::verifySignature()`

### What is verified exactly

The node checks that the signature is valid for:

- `st.transaction.transaction_hash`
- under `st.publicKey`

If that check fails, the transaction is rejected.

## 6. Public keys and private keys

### Private key

A secret value only the owner should know.
Used to create signatures.

### Public key

A shareable value others can use to verify signatures.

Axis defines both types in `common.h`:

- `SecretKey`
- `PublicKey`

### Important limitation

The current repository defines these types but does not implement a full end-user wallet or key-management workflow.

## 7. Address generation

An address is a shorter identifier derived from a public key.

Axis computes addresses using:

```cpp
Addr computeAddress(const PublicKey& pk)
```

This hashes the public key and stores the result in a 20-byte address type.

### Why this is useful

- addresses are shorter than public keys,
- users and systems can refer to ownership using addresses,
- the node can derive the expected sender address from the submitted public key.

## 8. Sender verification logic

One subtle but important rule exists in `handleCreateTransaction()`.

After deserializing the request, the node checks:

```cpp
computeAddress(signedTransaction.publicKey) == signedTransaction.transaction.sender
```

### Why this matters

Without this rule, a malicious client could claim any sender address while submitting their own public key.

This check binds:

- claimed sender address,
- actual cryptographic identity.

## 9. Security assumptions in Axis

The code assumes:

- libsodium implementations are correct,
- collision resistance of the chosen hash is strong enough for educational use,
- private keys are kept secret by whoever constructs transactions,
- the same transaction content produces the same hash on both client and server.

## 10. What cryptography does not solve by itself

Cryptography helps prove integrity and authorization, but it does not automatically solve:

- economic policy,
- network consensus across many peers,
- denial-of-service protection,
- storage corruption handling,
- portability of binary formats.

Those require broader system design.

## 11. Example mental model

Suppose Alice wants to spend a UTXO.

1. She creates a transaction describing the spend.
2. She hashes the transaction content.
3. She signs that hash with her private key.
4. She sends the transaction, her public key, and the signature.
5. Axis derives Alice’s address from the public key.
6. Axis checks the inputs belong to that address.
7. Axis verifies the signature against the transaction hash.

If those checks succeed, the node accepts the transaction into the mempool.

## 12. Common beginner mistakes

### Mistake: “The hash proves ownership.”

No. The hash identifies content. The signature proves authorization.

### Mistake: “The address is the same thing as the public key.”

Not in Axis. The address is derived from the public key.

### Mistake: “If the signature is valid, the transaction is automatically valid.”

No. The node must also verify ownership, input existence, mempool conflicts, and value totals.

## 13. Summary

Cryptography in Axis is intentionally focused and understandable:

- hashes identify and commit to data,
- signatures prove authorization,
- addresses are derived from public keys,
- Merkle roots summarize transaction sets inside blocks.

That is enough to support a meaningful educational blockchain core.
