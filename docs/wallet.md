# Wallet

## 1. Important truth about the current codebase

Axis does **not** currently include a complete wallet implementation.

That means there is no finished in-repository component that fully handles:

- key generation,
- secure secret-key storage,
- address book management,
- automatic coin selection,
- transaction signing UX,
- balance presentation for end users.

However, the codebase does contain the building blocks needed for a wallet layer:

- `SecretKey` and `PublicKey` types,
- `Addr` address type,
- `computeAddress(const PublicKey&)`,
- signed-transaction verification,
- `GetUTXOs` query support,
- `CreateTransaction` submission support.

So the best way to understand wallet behavior in Axis is to understand the **wallet responsibilities the current node expects a client to perform**.

## 2. What a wallet would do in this architecture

A wallet would be an external or future internal component responsible for:

1. generating or loading a keypair,
2. deriving the sender address from the public key,
3. asking the node for spendable UTXOs,
4. selecting which UTXOs to spend,
5. constructing transaction inputs and outputs,
6. computing the transaction hash,
7. signing that hash with the secret key,
8. sending a `CreateTransaction` packet to the node.

## 3. Key-related types in Axis

Defined in `axis/include/axis/core/common.h`:

- `SecretKey`
- `PublicKey`
- `Addr`
- `Signature`

### Meaning

- `SecretKey`: private signing key; must remain secret.
- `PublicKey`: shareable verification key.
- `Addr`: 20-byte address derived from the public key.
- `Signature`: detached digital signature over transaction hash.

## 4. Address derivation

Axis derives wallet addresses with:

```cpp
Addr computeAddress(const PublicKey& pk)
```

This hashes the public key into a 20-byte address.

### Why this matters for a wallet

The wallet must ensure that the `sender` field placed into a transaction is exactly the address derived from the wallet’s public key.

Otherwise, the node will reject the transaction with:

- `SenderPublicKeyMismatch`

## 5. Balance discovery from a wallet perspective

A wallet does not directly ask “what is my numeric balance?” through a completed balance API in current Axis.

Instead, it asks for UTXOs using `GetUTXOs`.

The node returns:

- a list of spendable input references,
- the total coin sum.

### Why this is useful

A wallet needs the individual spendable outputs, not just the total, because transactions spend concrete outputs.

## 6. How a wallet would create a transaction

### Step 1: gather spendable outputs

Send `GetUTXOs` for the wallet address.

### Step 2: choose inputs

Pick one or more UTXOs whose total value covers the desired payment and any change pattern.

### Step 3: construct outputs

Common pattern:

- one output to receiver,
- one output back to sender as change.

### Step 4: set transaction fields

Populate:

- `sender`
- `receiver`
- `coins`
- `inputs`
- `outputs`
- `timestamp`

### Step 5: hash the transaction

The wallet must hash fields in the same order used by `Transaction::computeTransactionHash(uint64_t)`.

### Step 6: sign the hash

Use the secret key to create a detached signature.

### Step 7: send `CreateTransaction`

Include the public key and signature alongside the transaction content.

## 7. What the node verifies about wallet-created transactions

When a wallet submits a transaction, the node checks:

- the payload format is valid,
- `computeAddress(publicKey) == sender`,
- inputs exist,
- inputs belong to the derived sender address,
- inputs are not duplicated,
- inputs cover outputs,
- signature verifies,
- the transaction is not already in mempool,
- the referenced inputs are not already reserved by another mempool entry.

## 8. What is missing for a real wallet product

A production-quality wallet would additionally need:

- secure secret-key encryption at rest,
- seed phrase or backup strategy,
- address formatting and checksum encoding,
- transaction fee logic,
- coin selection strategy,
- transaction history display,
- user confirmation flows,
- recovery and export/import tools.

None of these exist in the current repository.

## 9. Security considerations

If you add a wallet layer later:

- never log secret keys,
- never store raw secret keys in world-readable files,
- zero memory for secrets where appropriate,
- consider deterministic key generation and backup strategy,
- validate remote node responses before trusting them blindly.

## 10. Beginner summary

In current Axis terms, “wallet” mostly means:

- hold a keypair,
- derive an address,
- ask the node for spendable outputs,
- build and sign transactions,
- submit them to the node.

The node already validates that process, but the user-facing wallet experience has not yet been built.
