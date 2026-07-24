# Cryptography

This document describes the cryptographic primitives used in Axis.

## Overview

Axis uses exactly two cryptographic operations:

| Operation | Algorithm | Implementation |
|-----------|-----------|----------------|
| Hashing | Blake2b (256-bit) | libsodium `crypto_generichash` |
| Digital signatures | Ed25519 | libsodium `crypto_sign_*` |

There is no encryption, no key exchange, and no TLS. Network communication
is unencrypted (the assumption is that validation happens on the receiving
end — forging a packet cannot steal coins because forging a signature is
infeasible).

## Dependency: libsodium

[libsodium](https://doc.libsodium.org/) is a modern, portable, cross-platform
cryptographic library. Axis uses it via a single header
(`include/axis/crypto.h`) and a thin wrapper (`src/crypto.cpp`).

### Why libsodium?

- Well-audited, constant-time implementations
- Simple API (no complex configuration)
- Available on all platforms (Linux, macOS, Windows, BSD)
- Ship-stable API

### Linking

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)
target_link_libraries(axis_core PRIVATE ${SODIUM_LINK_LIBRARIES})
```

## Hashing (Blake2b)

### API

```cpp
Hash blake2b(std::span<const uint8_t> data);
```

- **Input:** any byte span
- **Output:** `Hash` = `std::array<uint8_t, 32>` (256 bits)
- **Underlying:** `crypto_generichash(out, outlen, in, inlen, NULL, 0)`

The null key (last two parameters) means no keyed hashing — this is plain
Blake2b, not Blake2b-MAC.

### Where hashing is used

| Location | What is hashed | Why |
|----------|---------------|-----|
| `Transaction::compute_hash` | All inputs + outputs + timestamp | Create txid |
| `Block::compute_hash` | 80-byte block header | Create block hash |
| `compute_block_merkle_root` | Txid pairs | Build Merkle tree |
| `derive_address` | Public key (32 bytes) → 20 bytes | Create address |

The Merkle tree hashes use the same `blake2b` function:

```cpp
Hash compute_block_merkle_root(const std::vector<Transaction>& txs) {
    std::vector<Hash> hashes;
    for (const auto& tx : txs)
        hashes.push_back(tx.txid());
    while (hashes.size() > 1) {
        // ... pair and hash ...
        next.push_back(blake2b(w.buf));
    }
    return hashes.empty() ? Hash{} : hashes[0];
}
```

### Why Blake2b and not SHA-256?

- Faster than SHA-256 in software (no hardware acceleration needed)
- Same security level (256-bit output, collision resistance ~2^128)
- Variable output length: 20 bytes for addresses, 32 bytes for hashes
- Simpler API than SHA-3

## Digital signatures (Ed25519)

Ed25519 is an elliptic-curve signature scheme using Curve25519. It was
designed by Daniel J. Bernstein.

### Key properties

- **Public key:** 32 bytes
- **Private key:** 64 bytes (32 bytes seed + 32 bytes derived public key)
- **Signature:** 64 bytes (32 bytes R + 32 bytes S)
- **Security level:** ~128 bits
- **Deterministic:** No randomness needed for signing (unlike ECDSA)
- **Batch verification:** Multiple signatures can be verified faster than
  verifying each individually

### Key generation

```cpp
void generate_keypair(PublicKey& pk, PrivateKey& sk) {
    crypto_sign_keypair(pk.data(), sk.data());
}
```

The private key in libsodium is actually 64 bytes: the first 32 bytes
are the seed, the last 32 bytes are the cached public key (for faster
signing).

### Signing

```cpp
Signature sign_msg(const PrivateKey& sk, const Hash& msg) {
    Signature sig;
    crypto_sign_detached(sig.data(), NULL, msg.data(), msg.size(), sk.data());
    return sig;
}
```

This produces a **detached** signature (just the 64-byte signature, not
the message + signature concatenated).

### Verification

```cpp
bool verify_sig(const PublicKey& pk, const Hash& msg, const Signature& sig) {
    return crypto_sign_verify_detached(
        sig.data(), msg.data(), msg.size(), pk.data()) == 0;
}
```

Returns `true` if the signature is valid for the given message under the
given public key.

### Deterministic signatures

Ed25519 signatures are deterministic: signing the same message with the same
key always produces the same signature. This is intentional — it prevents
randomness failures that could leak the private key.

The downside is that anyone who sees two transactions with the same signature
knows they were signed by the same key. This is not a privacy concern for
Axis's current use case.

### What is signed?

In Axis, the **txid** is what gets signed. The txid covers all inputs, all
outputs, and the timestamp. Signing the txid means:

- You cannot change the inputs (amount spent)
- You cannot change the outputs (where coins go)
- You cannot change the timestamp
- You cannot forge a signature without the private key

The wallet then sends a `SignedTransaction`:

```cpp
struct SignedTransaction {
    Transaction tx;
    PublicKey pubkey;
    Signature sig;
};
```

The receiver (a full node) verifies:

1. `derive_address(pubkey) == utxo_owner` — you own what you're spending
2. `verify_sig(pubkey, tx.txid(), sig)` — you authorized this exact tx

## Address derivation

```cpp
Address derive_address(const PublicKey& pk) {
    Address addr{};
    crypto_generichash(addr.data(), addr.size(),
                       pk.data(), pk.size(), nullptr, 0);
    return addr;
}
```

A 20-byte Blake2b hash of the public key. This is a **one-way function**:
you cannot recover the public key from the address. Only when spending do
you reveal the public key.

## Security model

### What Axis's crypto protects against

| Threat | How it's prevented |
|--------|-------------------|
| Spending someone else's coins | Signature verification + address derivation |
| Changing a transaction after signing | Signature covers the txid |
| Double-spending | UTXO set tracks spent outputs |
| Block tampering | Hash chain links each block to previous |
| Forging a block | Proof of Work (difficulty) |

### What Axis does NOT protect against

| Threat | Why it's not addressed |
|--------|----------------------|
| Eavesdropping | Network traffic is unencrypted. A passive attacker can see all transactions before they're confirmed. |
| Man-in-the-middle | No TLS. An active attacker on the network path can drop or modify packets (but modified packets will fail validation). |
| Sybil attacks | No peer discovery or identity. |
| 51% attacks | A single attacker with more hash power than the network can reorganize the chain. |
| Replay attacks | No per-input sequence number prevents the same signed transaction from being rebroadcast. (Mitigated by mempool dedup.) |

### Why no encryption?

Axis assumes **validation-based security**: every message is independently
verifiable. An encrypted but invalid transaction is still invalid. An
unencrypted but valid transaction is still valid. Encryption adds
complexity without improving consensus security.

True, eavesdroppers can see pending transactions before they're confirmed.
In production, this would be addressed by encryption at the transport layer
(TLS or Noise protocol) or by making transactions indistinguishable from
random data.
