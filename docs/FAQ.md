# FAQ

## Is Axis production cryptocurrency software?

No. It is educational. It lacks P2P networking, fork choice, robust consensus, authentication, rate limiting, payload hardening, wallet security, and many validation rules.

## What ports does it use?

Current `src/main.cpp` uses TCP port `8889` and HTTP/WebSocket port `8080`.

## Why does `/api/transaction` require `rawTx` instead of JSON fields?

Because validation needs public key and signature. The current implementation reuses the TCP `CreateTransaction` binary payload and wraps it as hex.

## Does `Transaction::serialize()` include signatures?

No. It includes txid, timestamp, inputs, and outputs only.

## Does the mempool survive restart?

Yes, pending transaction bodies are stored in the `pool` LevelDB database. However, public keys and signatures are not persisted, and pool entries are not revalidated on startup.

## Does Axis mine blocks?

No. External clients query tip/difficulty/pool, mine candidate headers, and submit `CreateBlock` packets.

## Are block rewards enforced?

No. `MINER_REWARD` exists in `Chain`, but `CreateBlock` currently accepts the coinbase amount supplied in the payload if all other checks pass.

## Does Axis support forks?

No. A block must reference the current tip hash.

## Why do docs mention both old and current API behavior?

The canonical docs are the TitleCase files in `docs/`. Some legacy lower-case docs and the original README may contain older statements; source and these generated docs reflect the current implementation.
