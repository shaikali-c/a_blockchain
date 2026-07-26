# Crypto Functions

Source: `include/axis/crypto.h`, `src/crypto.cpp`

## Functions

| Function | Purpose | Caller examples |
| --- | --- | --- |
| `blake2b(data)` | 32-byte hash primitive. | Transaction hash, block hash, Merkle nodes, tests. |
| `compute_merkle_root(leaves)` | Block transaction root. | `Block` construction, tests. |
| `derive_address(pk)` | Public key to 20-byte address. | `Chain::add_tx()`. |
| `verify_sig(pk,msg,sig)` | Validate Ed25519 detached signature. | `Chain::add_tx()`. |
| `sign_msg(sk,msg)` | Create Ed25519 detached signature. | Available to clients/tests, not used by daemon path. |

## Internal Helper

`combine_hash(a,b)` concatenates two 32-byte hashes and returns Blake2b of the 64-byte pair. It is file-local in `src/crypto.cpp`.

## Ownership and Side Effects

All crypto helpers are stateless and return by value. They require libsodium to have been initialized by the process.
