# Mining and Block Creation

Axis does not include an internal miner loop. Mining is performed by an external TCP client that queries node state and submits a `CreateBlock` packet.

## Miner Inputs

A miner can query:

| Query | Response | Use |
| --- | --- | --- |
| `GetTip` | current tip hash | block `prev_hash` |
| `GetDifficulty` | leading-zero byte count | target construction |
| `GetPool` | pending txids | candidate transaction selection |

The miner must also choose:

- coinbase recipient address,
- coinbase reward amount,
- coinbase timestamp,
- block timestamp,
- nonce.

## Candidate Construction

The server's `CreateBlock` parser reconstructs final transaction order as:

1. coinbase transaction built from submitted coinbase fields,
2. each mempool transaction referenced by submitted txids in payload order.

The submitted `wire_merkle` must match the Merkle root of that reconstructed transaction list.

## Proof of Work

The block hash is `BlockHeader::hash()` over:

```text
prev_hash || merkle_root || timestamp || nonce
```

The server accepts if `blk.hash() <= chain.target()`.

## Submission Validation

`Server::on_create_block()` checks:

- all txids are in the pool,
- Merkle root matches,
- hash meets target,
- previous hash equals current tip hash.

It then calls `Chain::add_block()`.

## Coinbase and Reward

`Chain` defines:

```text
MINER_REWARD = 3 * UNITS
```

However, the current block submission path does not enforce that submitted coinbase reward equals `MINER_REWARD`. The coinbase amount supplied in the payload is used as-is when constructing the coinbase transaction.

## Mining Limitations

- No internal mining loop.
- No fee accounting.
- No enforced block reward.
- No max transaction count/block size.
- No stale block recovery beyond rejection by previous hash.
- No peer broadcast beyond local WebSocket notification.
