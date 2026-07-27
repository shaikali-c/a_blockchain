# Performance

Axis prioritizes clarity over indexing and optimization. The codebase is small and suitable for educational workloads.

## Complexity Summary

| Operation | Complexity | Notes |
| --- | --- | --- |
| `Transaction::compute_hash()` | O(inputs + outputs) | One pass through vectors. |
| `Transaction::serialize()` / deserialize | O(inputs + outputs) | Allocates/reserves vectors on read. |
| `compute_merkle_root(n)` | O(n) hashes | Uses current/next vectors, duplicates odd leaf. |
| `Block::serialize()` / deserialize | O(transactions + total tx bytes) | Serializes full transactions. |
| `Chain::load_blocks()` | O(blocks + transactions) | Replays all blocks into UTXO set. |
| `Chain::get_block(height)` | O(1) | Vector index. |
| `Chain::get_block(hash)` | O(number of blocks) | Linear scan, no hash index. |
| `Chain::get_blocks(start,count)` | O(count) | Copies block objects. |
| `Chain::get_utxos(address)` | O(total UTXOs) | Full scan, no address index. |
| `Chain::add_tx()` | O(inputs + outputs) average | Hash-map lookups; graph analysis flagged scans/contains in loops due repeated lookups. |
| `Chain::add_block()` | O(block txs + tx inputs/outputs) | Applies all txs and deletes mined pool entries. |
| Web JSON block rendering | O(block transactions) | Manual string construction. |
| WebSocket broadcast | O(connections) | Sends while holding mutex. |

## Hotspots from Code Graph

The indexed graph identifies higher cognitive complexity in:

- `WebServer::setup_routes()` — many route lambdas in one function.
- `json_escape()` — switch-based escaping.
- `Chain::add_tx()` — validation path with multiple loops and checks.
- `Server::on_create_block()` — parser/result handling and acceptance checks.
- `tx_error_str()` / `tx_error_to_string()` — switch mapping.

These are not necessarily problems at current scale, but they are the most likely places to refactor as functionality grows.

## Memory Ownership and Copies

- `Chain` read methods return copies of blocks/transactions to avoid exposing internal mutable state after locks are released.
- `Block` contains full `Transaction` objects; storing/copying blocks can copy all transaction vectors.
- `get_pool_txs()` returns a vector copy of all pending transactions.
- `get_blocks()` returns a vector copy of requested blocks.
- `WebServer` builds JSON strings through `std::ostringstream`, which is simple but not allocation-optimal.

## Storage Performance

- Startup replays all blocks to reconstruct UTXO state; there is no persisted UTXO snapshot.
- Block lookup by hash scans memory rather than using a database/index.
- Address UTXO lookup scans all UTXOs rather than maintaining an address index.
- LevelDB writes are individual `Put`/`Delete` operations; block acceptance does not use write batches.

## Networking Performance

- TCP sessions allocate a vector sized by each packet's declared payload size.
- There are no explicit maximum TCP payload sizes/counts.
- Crow is configured as multithreaded, but all shared chain access goes through `Chain` locks.
- WebSocket broadcast holds `ws_mutex_` while calling `send_text()` on each connection.

## Recommended Optimizations

1. Add explicit max payload sizes and count limits before vector allocation.
2. Add block hash index: `unordered_map<Hash, uint32_t>`.
3. Add address UTXO index for fast wallet queries (wallet implementations in companion repositories currently work around this).
4. Store UTXO snapshots or incremental indexes if chain grows.
5. Use LevelDB write batches for atomic and faster block/pool updates.
6. Refactor `setup_routes()` into route groups/helper classes.
7. Copy WebSocket connection pointers under lock, then send outside the mutex.
8. Use a JSON library or Crow JSON builders for large responses.
