# Glossary

| Term | Meaning in Axis |
| --- | --- |
| Address | 20-byte Blake2b-derived digest of an Ed25519 public key. |
| Block | Header plus full list of transactions. |
| Block hash | Blake2b hash of serialized `BlockHeader`. |
| Coinbase | Transaction with no inputs, used to create new coins in genesis and submitted blocks. |
| Difficulty | Number of leading zero bytes used to build the target; hardcoded to `3`. |
| Hash | 32-byte digest represented by `Hash`. |
| Mempool | Pending transactions in `pool_` and `pool` LevelDB. |
| Merkle root | Root hash computed from transaction txids with duplicate-last odd layer rule. |
| OutPoint | Reference to a previous output: txid plus index. |
| Pool spent | `pool_spent_` index of inputs reserved by pending transactions. |
| Target | 32-byte threshold; blocks are accepted if hash is lexicographically <= target. |
| Transaction ID | Blake2b hash of transaction inputs, outputs, and timestamp. |
| UTXO | Unspent transaction output stored in `utxo_`. |
| Writer/Reader | Custom native-endian binary serialization helpers. |
