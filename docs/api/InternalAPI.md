# Internal API Reference

This document summarizes the C++ API exposed inside the project. It includes public class methods and important private/helper functions.

## Types and Serialization (`include/axis/types.h`)

### Aliases

| Alias | Meaning |
| --- | --- |
| `Hash` | 32-byte digest. |
| `Address` | 20-byte address digest. |
| `PublicKey` | libsodium Ed25519 public key bytes. |
| `SecretKey` | libsodium Ed25519 secret key bytes. |
| `Signature` | libsodium detached signature bytes. |

### `Writer`

Append-only binary writer. Owns `std::vector<uint8_t> buf`.

| Method | Purpose | Side effects |
| --- | --- | --- |
| `put_bytes(data,len)` | Append raw bytes. | Grows `buf`. |
| `put_u8/u16/u32/u64/i32` | Append native integer bytes. | Grows `buf`. |
| `put_hash/put_addr/put_pk/put_sig` | Append fixed-size byte arrays. | Grows `buf`. |
| `put_span()` | Append span bytes. | Grows `buf`. |
| `put_str()` | Append string bytes without length. | Grows `buf`. |
| `str()` | Convert buffer to `std::string`. | Copies bytes. |

### `Reader`

Bounds-checking binary reader over `std::string_view`.

| Method | Purpose | Failure |
| --- | --- | --- |
| `remain()` | Bytes left. | None. |
| `check(n)` | Ensure `n` bytes remain. | Throws `runtime_error`. |
| `take_u8/u16/u32/u64` | Read native integer bytes. | Throws if short. |
| `take_hash/take_addr/take_pk/take_sig` | Read fixed byte arrays. | Throws if short. |
| `take_view(n)` | Return next `n` bytes as view. | Throws if short. |

### `Timestamp`

Wraps a `uint64_t` Unix timestamp in seconds. Provides `now()` and comparison operators.

## Crypto (`crypto.h` / `crypto.cpp`)

| Function | Why it exists | Complexity |
| --- | --- | --- |
| `blake2b(data)` | Unified 32-byte digest primitive for txids, block hashes, Merkle nodes, and addresses. | O(data length). |
| `compute_merkle_root(leaves)` | Computes block transaction root. Duplicates odd leaf. | O(n) hashes, O(n) memory. |
| `derive_address(pk)` | Binds Ed25519 pubkey to 20-byte spend address. | O(public key length). |
| `verify_sig(pk,msg,sig)` | Verifies transaction authorization. | libsodium Ed25519 cost. |
| `sign_msg(sk,msg)` | Helper for signing hashes. | libsodium Ed25519 cost. |

## Transaction API

| Method/function | Why it exists | Notes |
| --- | --- | --- |
| `Transaction(ins, outs, ts)` | Construct canonical transaction and compute txid. | Inputs/outputs moved in. |
| `Transaction(serialized)` | Restore from storage bytes. | Trusts serialized txid. |
| `txid()` | Read cached txid. | Returns const reference. |
| `is_coinbase()` | Distinguish no-input reward transactions. | True when `inputs.empty()`. |
| `serialize(Writer&)` / `serialize()` | Store transaction bytes. | Does not include signature/public key. |
| `deserialize(Reader&)` | Read transaction from a larger stream. | Throws on short input. |
| `pretty()` and `operator<<` | Human-readable debugging output. | Writes to stream. |

Private `compute_hash()` recomputes `txid_` from inputs, outputs, and timestamp. No public recompute API exists.

## Block API

| Method/function | Why it exists | Notes |
| --- | --- | --- |
| `BlockHeader::hash()` | Compute proof-of-work hash. | Blake2b over serialized header. |
| `BlockHeader::serialize()` / `deserialize()` | Header storage and hashing preimage. | 80 bytes. |
| `Block(prev, txs, ts, nonce)` | Construct block and compute Merkle/hash. | Transactions moved in. |
| `Block(serialized)` | Restore stored block. | Delegates to `deserialize`. |
| `header()` | Read header. | Const reference. |
| `hash()` | Read cached block hash. | Const reference. |
| `Block::serialize()` / `deserialize()` | Store/load block with full txs. | Throws on short input. |

Internal `compute_block_merkle_root()` maps each transaction to txid and calls `compute_merkle_root()`.

## Chain API

| Method | Purpose | Threading |
| --- | --- | --- |
| `Chain()` | Open DBs, load state, create genesis if needed, build target. | Construction only. |
| `~Chain()` | Default cleanup of DB unique_ptrs. | N/A. |
| `add_tx(st)` | Validate signed transaction and persist to mempool. | Unique lock. |
| `get_utxos(addr,out)` | Scan available UTXOs for address, excluding pool-spent. | Shared lock. |
| `tip()` / `tip_hash()` | Current tip block/hash. | Shared lock. |
| `height()` | Number of blocks. | Shared lock. |
| `get_difficulty()` / `target()` | Current PoW parameters. | Shared lock. |
| `get_block(height/hash)` | Block lookup by height or hash. | Shared lock. |
| `get_blocks(start,count)` | Paginated block copy. | Shared lock. |
| `get_pool_txs()` | Pending transaction copies. | Shared lock. |
| `pool_contains(txid)` / `get_pool_tx(txid)` | Mempool lookup for block assembly. | Shared lock. |
| `add_block(blk)` | Apply already-validated block, cleanup pool, persist. | Unique lock. |

Private helpers include `load_blocks`, `load_pool`, `rebuild_utxo`, `dump_utxo`, `apply_tx`, `build_target`, `store_block`, `create_genesis`, `block_key`, and `verify_block_header`. `verify_tx` and `verify_block` are declared but not implemented.

## Server API

`Server` owns Asio TCP serving.

| Method | Purpose |
| --- | --- |
| `Server(chain, port, events)` | Bind TCP acceptor and retain chain/events references. |
| `run()` | Start accept loop and run io_context. |
| `do_accept()` | Schedule async accepts. |
| `handle_client(sock)` | Coroutine session loop. |
| `handle_msg(type,payload,sock)` | Dispatch request messages. |
| `on_get_*` | Query handlers. |
| `on_create_tx()` | Parse, validate, persist, broadcast accepted tx. |
| `on_create_block()` | Parse, check, connect, broadcast accepted block. |
| `send()` | Serialize TCP response envelope. |
| `serialize_utxo_response()` | Binary UTXO response payload. |
| `serialize_tx_response()` | Binary tx result payload. |

Internal parser helpers in `src/net.cpp`: `parse_create_tx_payload`, `parse_create_block_payload`, `tx_error_str`, `serialize_block_response`.

## WebServer API

| Method | Purpose |
| --- | --- |
| `WebServer(chain, port)` | Store references and register routes. |
| `run()` | Run Crow multithreaded server. |
| `stop()` | Stop Crow app. |
| `broadcast_new_tx(tx)` | Send `new_tx` event to WebSocket clients. |
| `broadcast_new_block(block)` | Send `new_block` event. |
| `setup_routes()` | Register all HTTP/WebSocket routes. |
| `broadcast_text(message)` | Send raw text to all WebSocket connections. |

Internal web helpers include JSON escaping/response builders, hex decoding, signed transaction parsing, JSON serializers for outpoints/outputs/transactions/blocks/events, and `parse_u32`.
