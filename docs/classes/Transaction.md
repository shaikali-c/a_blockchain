# `Transaction`, `OutPoint`, `TxOutput`, `SignedTransaction`

Source: `include/axis/tx.h`, `src/tx.cpp`

## `OutPoint`

Identifies one previous transaction output.

Fields:

- `Hash txid`
- `uint32_t index`

`operator==` is defaulted. A `std::hash<OutPoint>` specialization hashes the first machine word of the txid XORed with the index.

## `TxOutput`

Represents a spendable output.

Fields:

- `Address recipient`
- `uint64_t amount`

`operator==` is defaulted.

## `Transaction`

Fields:

| Field | Visibility | Meaning |
| --- | --- | --- |
| `txid_` | private | Cached transaction hash. |
| `inputs` | public | Outpoints consumed by this transaction. |
| `outputs` | public | Outputs created by this transaction. |
| `timestamp` | public | Unix seconds timestamp. |

Methods:

| Method | Purpose |
| --- | --- |
| `Transaction(ins, outs, ts)` | Construct and compute txid. |
| `Transaction(serialized)` | Deserialize from storage bytes. |
| `txid()` | Return cached txid. |
| `is_coinbase()` | True when there are no inputs. |
| `serialize(Writer&)` / `serialize()` | Write storage representation. |
| `deserialize(Reader&)` | Read from binary stream. |
| `pretty()` | Human-readable debug output. |
| `operator==` | Compares txids only. |

Private `compute_hash()` writes all inputs, outputs, and timestamp into a `Writer` and hashes the bytes with Blake2b.

## `SignedTransaction`

Submission wrapper:

- `Transaction tx`
- `PublicKey pubkey`
- `Signature sig`

It is used by `Chain::add_tx()`. It is not persisted as a whole; the pool stores only `Transaction`.

## Why It Exists

The transaction layer intentionally knows how to describe and serialize a transaction, but it does not know whether the transaction is valid against a chain. This separation keeps UTXO ownership, signature verification, and mempool rules inside `Chain`.

## Side Effects

Constructors and serialization allocate memory. `pretty()` writes to an output stream. Transaction methods do not touch global chain state or databases.

## Future Improvements

- Make transaction fields private to preserve txid invariants.
- Include input/output counts in the hash preimage or define a canonical hash serialization.
- Verify serialized txid on deserialize.
- Store signatures if replay/revalidation of mempool entries is required.
