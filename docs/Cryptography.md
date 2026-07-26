# Cryptography

Axis uses libsodium for hashing, address derivation, signatures, and hex conversion.

## Initialization

`src/main.cpp` and `tests/core_serialization_tests.cpp` call `sodium_init()` before using crypto helpers. If initialization fails in `axisd`, the process logs an error and returns `1`.

## Hash Type

`Hash` is:

```text
std::array<uint8_t, 32>
```

`blake2b()` uses `crypto_generichash()` with a 32-byte output and no key.

## Address Type

`Address` is:

```text
std::array<uint8_t, 20>
```

`derive_address(pubkey)` hashes the Ed25519 public key with `crypto_generichash()` and asks libsodium to produce 20 output bytes. This is similar to a hash160-style address in spirit, but it is a direct Blake2b-derived 20-byte digest, not Base58, Bech32, or RIPEMD160.

## Signatures

| Type | Definition |
| --- | --- |
| `PublicKey` | `std::array<uint8_t, crypto_sign_PUBLICKEYBYTES>` |
| `SecretKey` | `std::array<uint8_t, crypto_sign_SECRETKEYBYTES>` |
| `Signature` | `std::array<uint8_t, crypto_sign_BYTES>` |

`sign_msg(secret_key, hash)` calls `crypto_sign_detached()` over the 32-byte hash.

`verify_sig(public_key, hash, signature)` calls `crypto_sign_verify_detached()`.

Transactions are verified by checking the signature over `Transaction::txid()`.

## Merkle Root

`compute_merkle_root(leaves)` uses a Bitcoin-like duplicate-last rule:

1. If there are no leaves, return an all-zero hash.
2. Copy leaves into a working vector.
3. While more than one hash remains:
   - if odd count, duplicate the last hash,
   - hash each adjacent pair as `blake2b(left || right)`,
   - replace current layer with next layer.
4. Return the final hash.

Time complexity is O(n), and memory is O(n) for the current/next vectors.

## Block Hash

`BlockHeader::hash()` is Blake2b over:

```text
prev_hash || merkle_root || timestamp || nonce
```

Timestamp and nonce use native integer byte representation through `Writer`.

## Transaction Hash

`Transaction::compute_hash()` is Blake2b over:

```text
inputs || outputs || timestamp
```

It does not include public key or signature. This allows the same transaction body to be signed externally, but it also means public key ownership is enforced by checking the referenced UTXO recipients, not by embedding sender identity into the transaction body.

## Hex Encoding

`to_hex()` and `from_hex<N>()` use libsodium `sodium_bin2hex()` and `sodium_hex2bin()`.

- Hash hex is 64 characters.
- Address hex is 40 characters.
- Invalid length or invalid characters throw `std::runtime_error` in `from_hex()`.

## Cryptographic Limitations

- No domain separation between transaction hashes, block hashes, Merkle internal nodes, and address derivation.
- No chain ID or replay-protection field in transaction hash preimage.
- No signature malleability policy beyond libsodium verification.
- No secure key storage or wallet implementation in this repository.
- No authenticated network messages.
