# UTXO Model

This document explains the UTXO model in general and exactly how Axis implements it.

## 1. What is the UTXO model?

UTXO means **Unspent Transaction Output**.

Instead of tracking balances directly as a single number per account, a UTXO system tracks many individual spendable outputs.

An output becomes spendable when a transaction creates it.
An output stops being spendable when a later transaction uses it as an input.

## 2. Analogy

Imagine cash in paper envelopes.

- Each envelope contains some money.
- If the envelope belongs to you, you can spend it.
- To make a payment, you hand over one or more envelopes.
- The system then creates new envelopes for the receiver and possibly one “change” envelope back to you.

A UTXO is like one of those envelopes.

## 3. Axis data types involved

## `UTXO`

Defined in `axis/include/axis/blockchain/transaction.h`.

Fields:

- `owner`: 20-byte address that owns the output,
- `coins`: amount stored in the output.

## `Input`

Also defined in `transaction.h`.

Fields:

- `transaction_hash`: which earlier transaction created the output,
- `output_index`: which output inside that transaction is being referenced.

An `Input` does not contain money by itself. It is only a pointer to a previously created output.

## 4. How Axis identifies a UTXO

Axis uses a string key format:

```text
<transaction_hash_hex>:<output_index>
```

Example:

```text
6f8c...9a:1
```

This means:

- transaction hash `6f8c...9a`,
- output index `1`.

This key is produced by:

- `Input::getUTXOKey()`

and parsed by:

- `parseUTXOKey()`

## 5. Where the UTXO set lives

The active UTXO set is stored in memory in:

- `Blockchain::utxo`

Type:

```cpp
std::unordered_map<std::string, UTXO>
```

The map key is the text UTXO key.
The map value is the actual unspent output.

## 6. How the UTXO set is built

Axis rebuilds the UTXO set by replaying stored blocks during startup.

That happens inside:

- `Blockchain::loadBlocks()`
- `Blockchain::updateUTXO(const Transaction&)`

### Replay process

For each stored block:

1. deserialize the block,
2. iterate through each transaction,
3. add the transaction to the `transactions` map,
4. apply `updateUTXO(transaction)`.

## 7. How `updateUTXO()` works

`updateUTXO()` does two things:

### Remove spent outputs

For every input in the transaction:

- compute its UTXO key,
- erase that key from the `utxo` map.

### Add newly created outputs

For every output in the transaction:

- build a key using the new transaction hash and the output index,
- insert the new `UTXO` into the `utxo` map.

This is the core accounting step.

## 8. How ownership is checked

When validating a transaction, Axis does not trust the transaction’s claimed sender by itself.

It:

1. derives an address from the submitted public key,
2. checks that each referenced UTXO belongs to that derived address.

This check happens in:

- `Blockchain::verifyInputs()`

That means the node effectively asks:

> “Do all referenced spendable outputs belong to the address derived from this public key?”

If not, the transaction is rejected.

## 9. Why a transaction may have multiple inputs

A single UTXO may not contain enough coins for a payment.

So a transaction can combine several inputs.

Axis sums all referenced input values and ensures:

- total inputs >= total outputs

It does this in `verifyInputs()`.

## 10. Change outputs

In UTXO systems, if your inputs are larger than your payment, the difference usually comes back to you as change.

Axis supports that naturally through multiple outputs.

Example:

- Input total: 10 coins
- Receiver output: 7 coins
- Change output back to sender: 3 coins

`verifyInputs()` allows this because it checks only that total inputs are at least total outputs.

## 11. Double-spend prevention in Axis

Axis prevents double spending in two places.

### Against confirmed state

If an input references a UTXO that is no longer in the `utxo` map, the output is already spent or never existed.

### Against the mempool

If another pending transaction has already reserved the same input, Axis rejects the new one.

This is tracked in:

- `Blockchain::mempoolInputs`

## 12. Address query flow

The `GetUTXOs` request asks the node:

> “Show me all currently spendable outputs owned by this address.”

Axis answers by scanning the entire `utxo` map in:

- `Blockchain::findAddressUtxos()`

It returns:

- matching inputs,
- total coin amount.

## 13. Why Axis stores UTXOs in memory instead of querying blocks every time

A blockchain can contain many historical transactions. Re-scanning all history for every balance lookup would be slow.

The UTXO set is a compact “current spendable state” index.

### Benefit

Fast validation and address queries.

### Tradeoff

The node must keep the UTXO set correct at all times.

## 14. Limitations of the current implementation

### No dedicated UTXO database

The UTXO set is reconstructed from blocks at startup instead of being stored in its own persistent index.

### Full scan for address lookup

`findAddressUtxos()` scans every UTXO instead of using an address-indexed structure.

### No coin selection strategy

The node can validate submitted inputs, but it does not currently build transactions for a wallet automatically.

## 15. Example

Assume transaction `T1` creates two outputs:

- output 0: address Alice gets 5
- output 1: address Bob gets 8

Axis creates keys:

```text
T1_hash:0
T1_hash:1
```

Now Bob wants to pay Carol 6.

Bob submits a transaction with:

- input: `T1_hash:1`
- outputs:
  - Carol gets 6
  - Bob gets 2 change

After `updateUTXO()`:

- `T1_hash:1` is removed,
- new outputs for Carol and Bob are added under the new transaction hash.

## 16. Summary

The UTXO model in Axis is simple but important:

- outputs are the spendable pieces of value,
- inputs point to previous outputs,
- the `utxo` map is the node’s current spendable state,
- transaction validation is mostly about proving the submitted inputs are valid, owned, unique, and not already reserved.
