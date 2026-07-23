# Developer Guide

This guide is for contributors who want to modify or extend Axis safely.

## 1. Development mindset

Axis is small enough that changes can seem easy, but there are several hidden contracts:

- binary formats must stay consistent,
- transaction hash field order matters,
- UTXO updates define ledger correctness,
- mempool reservations prevent double spends,
- startup replay reconstructs authoritative state.

When changing code, always think about those contracts first.

## 2. Best first reading order

1. `axis/include/axis/core/common.h`
2. `axis/include/axis/blockchain/transaction.h`
3. `axis/src/blockchain/transaction.cpp`
4. `axis/include/axis/blockchain/block.h`
5. `axis/src/blockchain/block.cpp`
6. `axis/include/axis/blockchain/blockchain.h`
7. `axis/src/blockchain/blockchain.cpp`

## 3. Build and test loop

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

If you touch serialization, add or update tests immediately.

## 4. Safe change areas for beginners

### Good starter tasks

- add more tests,
- improve logging messages,
- add documentation,
- implement a missing query handler,
- add helper functions around packet parsing.

### Higher-risk tasks

- changing transaction hash field order,
- changing serialization layout,
- changing genesis values,
- changing UTXO update rules,
- introducing threads.

## 5. How to reason about correctness

For transaction-related changes, ask these questions:

1. Does this preserve transaction-hash determinism?
2. Could this accept an invalid spend?
3. Could this reject a valid spend incorrectly?
4. Could this break replay from stored blocks?
5. Could this break packet compatibility?

## 6. Adding a new network message

Recommended process:

1. add or confirm enum entry in `PayloadType`,
2. define exact payload layout in docs,
3. add parser helper if needed,
4. add handler method to `Blockchain`,
5. update `handlePayload()`,
6. add serialization/deserialization tests,
7. add client example bytes in documentation.

## 7. Adding a new persistent structure

Before storing new data, decide:

- what key format you will use,
- whether iteration order matters,
- whether the data is canonical or derived,
- how startup recovery should rebuild in-memory state,
- whether writes must be atomic across multiple records.

## 8. Extending block handling

If you implement full block acceptance, make sure to include:

- block storage to `blocksDB`,
- `blocks` append,
- `blocksMap` update,
- confirmed `transactions` update,
- UTXO updates for each included transaction,
- mempool removal for included transactions,
- mempool input reservation cleanup.

Missing any one of those can leave the node inconsistent.

## 9. Testing recommendations

### For serialization changes

Add tests for:

- round-trip equality,
- malformed truncation,
- invalid counts,
- trailing bytes.

### For transaction validation changes

Add tests for:

- duplicate inputs,
- insufficient inputs,
- wrong owner,
- bad signature,
- mempool conflict.

### For block changes

Add tests for:

- bad Merkle root,
- wrong previous hash,
- invalid coinbase,
- insufficient difficulty.

## 10. Known design constraints

### Singleton architecture

Because `Blockchain` is a singleton, unit isolation is harder. Avoid introducing hidden cross-test contamination if you later add stateful tests.

### Global Asio objects

The current networking setup is simple but not easily injectable. Consider refactoring if protocol testing expands.

### Native binary layout

Be cautious if you test across architectures or try to write clients in other languages.

## 11. Refactoring advice

If you refactor, preserve behavior first.

A good order is:

1. add tests,
2. move code without changing logic,
3. re-run tests,
4. only then change behavior.

## 12. Suggested future module split

If the project becomes larger, a cleaner structure could be:

- `chain/` for block/transaction/state rules,
- `mempool/` for pending transaction policy,
- `protocol/` for packet parsing/encoding,
- `storage/` for persistence,
- `node/` for orchestration,
- `wallet/` for key and transaction construction.

## 13. Common contributor mistakes

- assuming declared functions are fully implemented,
- forgetting mempool persistence after acceptance,
- forgetting to recompute hashes after deserialization changes,
- changing serialization without updating tests and docs,
- adding threaded code without protecting shared state.

## 14. Checklist before opening a PR or saving a major change

- [ ] Build succeeds
- [ ] Tests pass
- [ ] Serialization docs updated
- [ ] Packet docs updated if protocol changed
- [ ] New error paths documented
- [ ] No accidental change to genesis constants
- [ ] UTXO state transitions still make sense
