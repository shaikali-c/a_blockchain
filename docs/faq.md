# FAQ

## Is Axis a full cryptocurrency node?

No. It is an educational blockchain node foundation with several important pieces implemented, but it is not a complete production-ready cryptocurrency.

## Does Axis mine blocks?

Not fully in the current visible code. It has difficulty-target logic and block verification helpers, but no complete mining loop that searches for nonces and commits new blocks.

## Does Axis have a wallet?

Not in the repository as a finished feature. It defines key-related types and verifies signatures, but it does not provide a full wallet workflow or user-facing key management.

## Does Axis support peer-to-peer synchronization?

No. There is no complete peer discovery or chain synchronization flow in the current code.

## What network API is available right now?

The practical implemented message paths are:

- `GetUTXOs`
- `CreateTransaction`

Several other payload types are declared but not fully implemented.

## How are balances stored?

Balances are not stored as a single number per address. Axis uses a UTXO model, where spendable outputs are tracked individually.

## What database does it use?

LevelDB.

There are two database directories:

- `blocks/` for blocks,
- `pool/` for pending mempool transactions.

## Is the binary protocol portable across architectures?

Not reliably. The current code writes raw native integer bytes and even stores `size_t` in block serialization.

## Why is the mempool persisted?

So pending transactions can survive a process restart.

## Why does the node rebuild the UTXO set at startup?

Because confirmed spendable state is derived by replaying stored block transactions.

## Why are addresses only 20 bytes?

The project derives addresses by hashing the public key into a fixed 20-byte array. This is a design choice for compactness, not a universal blockchain rule.

## Why does `CreateTransaction` include both sender address and public key?

The sender address is the claimed owner identity. The public key is used to derive the actual cryptographic identity and verify the signature. The server checks that both match.

## What does `coins` mean if outputs also contain amounts?

`coins` expresses the primary transaction amount field, but the real state transition is determined by the outputs. Maintainers should be careful not to assume `coins` alone defines total spending behavior.

## Why does the project use a singleton?

Likely for simplicity. It reduces setup code, but it also concentrates responsibilities and makes testing harder.

## Where should I start if I want to contribute?

Start with:

1. `docs/getting_started.md`
2. `docs/architecture.md`
3. `docs/transaction_lifecycle.md`
4. `docs/class_reference.md`
5. `docs/function_reference.md`
