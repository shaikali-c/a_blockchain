# Function Reference

This reference documents every project-defined function/method category and its role. Very small one-line accessors are grouped where appropriate.

## `types.h`

### `Writer`

- `put_bytes`: appends raw bytes into `buf`; foundational primitive used by all other writer methods.
- `put_u8`, `put_u16`, `put_u32`, `put_u64`, `put_i32`: append native integer bytes; used for protocol/storage fields.
- `put_hash`, `put_addr`, `put_pk`, `put_sig`: append fixed-size byte arrays.
- `put_span`: append arbitrary bytes from a span.
- `put_str`: append string bytes without a length prefix; callers must encode length separately if needed.
- `str`: returns a `std::string` copy of `buf`.

### `Reader`

- `Reader(std::string_view)`: creates a cursor over immutable bytes.
- `remain`: returns `data.size() - offset`.
- `check`: throws if a requested read would exceed remaining bytes.
- `take_u8`, `take_u16`, `take_u32`, `take_u64`: read native integer bytes and advance offset.
- `take_hash`, `take_addr`, `take_pk`, `take_sig`: read fixed byte arrays and advance offset.
- `take_view`: returns a view into the next `n` bytes and advances offset.

### `Timestamp`

- constructors: default or explicit `uint64_t`.
- `now`: returns current Unix time in seconds.
- comparison operators: compare the wrapped `value`.

## `util.h`

- `logging::info`, `logging::err`, `logging::reject`: write prefixed messages to `std::cout`.
- `to_hex`: uses libsodium to convert fixed byte arrays to lowercase hex.
- `from_hex`: validates exact length and converts lowercase/valid hex to bytes.
- `short_hex`: currently returns full hash hex.
- `short_addr`: returns shortened address display.
- `format_amount`: formats integer base units as fixed six-decimal AXIS amount by default.
- `format_timestamp`: formats UTC timestamp for display.

## `crypto.cpp`

- `blake2b`: returns a 32-byte libsodium generic hash for input bytes.
- `combine_hash`: file-local helper that hashes two concatenated hashes; used by Merkle construction.
- `compute_merkle_root`: computes a duplicate-last Merkle root from hash leaves.
- `derive_address`: derives a 20-byte address from a public key using `crypto_generichash`.
- `verify_sig`: verifies an Ed25519 detached signature over a hash.
- `sign_msg`: creates an Ed25519 detached signature over a hash.

## `tx.cpp`

- `Transaction::compute_hash`: private method that derives `txid_` from inputs, outputs, and timestamp.
- `Transaction::Transaction(vector, vector, Timestamp)`: moves fields in and computes hash.
- `Transaction::Transaction(serialized)`: deserializes from bytes.
- `Transaction::serialize(Writer&)`: writes txid, timestamp, inputs, and outputs.
- `Transaction::serialize()`: returns serialized bytes as `std::string`.
- `Transaction::deserialize(Reader&)`: reads a transaction body from a reader.
- `Transaction::pretty`: writes a human-readable transaction summary.
- `operator<<(OutPoint)`: prints an outpoint.
- `operator<<(TxOutput)`: prints an output.
- `operator<<(Transaction)`: delegates to `pretty`.

## `block.cpp`

- `BlockHeader::hash`: hashes serialized header.
- `BlockHeader::serialize`: writes prev hash, Merkle root, timestamp, nonce.
- `BlockHeader::deserialize`: reads those fields.
- `compute_block_merkle_root`: file-local helper that gathers transaction txids and calls `compute_merkle_root`.
- `Block::Block(prev, txs, ts, nonce)`: constructs header, computes Merkle root and cached hash.
- `Block::Block(serialized)`: deserializes from bytes.
- `Block::serialize(Writer&)`: writes header and length-prefixed serialized transactions.
- `Block::serialize()`: returns serialized block bytes.
- `Block::deserialize(Reader&)`: reads header and transaction list, recomputes cached hash.
- `operator<<(BlockHeader)`: prints header fields.
- `operator<<(Block)`: prints block summary and transaction list.

## `chain.cpp`

- `hex_key`: file-local helper converting txid hash to lowercase hex for pool DB keys.
- `Chain::Chain`: opens DBs, loads blocks/pool, creates genesis if empty, logs state, builds target.
- `Chain::~Chain`: default cleanup.
- `tip`, `tip_hash`, `height`, `get_difficulty`, `target`: locked read accessors.
- `get_block(height)`: locked vector-index lookup.
- `get_block(hash)`: locked linear hash lookup.
- `get_blocks`: locked paginated block copy.
- `load_blocks`: iterates block DB, deserializes blocks, applies txs to UTXO, fills `blocks_`.
- `load_pool`: iterates pool DB, deserializes txs, fills `pool_` and `pool_spent_`.
- `apply_tx`: removes spent inputs and inserts new outputs into `utxo_`.
- `create_genesis`: creates and persists hardcoded genesis block.
- `rebuild_utxo`: clears and rebuilds UTXO by replaying loaded blocks; currently unused.
- `dump_utxo`: logs sorted UTXO contents.
- `add_tx`: validates signed transaction, inserts into mempool, persists to pool DB.
- `get_utxos`: scans UTXO set for an address excluding pending spends.
- `build_target`: constructs `target_` from `difficulty_`.
- `store_block`: writes serialized block to blocks DB using next height key.
- `block_key`: formats a 10-digit height key.
- `pool_contains`: locked txid membership check.
- `get_pool_tx`: locked tx copy lookup; throws if absent.
- `get_pool_txs`: locked vector copy of pending txs.
- `verify_block_header`: checks previous hash, timestamp, and leading zero bytes; currently unused.
- `add_block`: applies accepted block, removes mined pool txs, stores block, increments height.

Declared but not implemented:

- `verify_tx`
- `verify_block`

## `net.cpp`

- `tx_error_str`: maps `TxError` values to protocol reason strings.
- `serialize_block_response`: encodes block result response payload.
- `parse_create_tx_payload`: reads TCP create-transaction payload into a parsed struct.
- `parse_create_block_payload`: reads block proposal, creates coinbase, resolves mempool txids.
- `Server::Server`: binds TCP acceptor and stores references/callbacks.
- `Server::run`: starts accepting and runs io context.
- `Server::do_accept`: schedules async accepts and spawns sessions.
- `Server::handle_client`: coroutine packet read loop.
- `Server::handle_msg`: dispatches handled message types.
- `Server::on_get_difficulty`: sends current difficulty.
- `Server::on_get_pool`: sends txids for pending transactions.
- `Server::on_get_tip`: sends tip hash.
- `Server::on_get_utxos`: parses address and sends UTXO list.
- `Server::on_create_tx`: parses signed transaction, calls `Chain::add_tx`, broadcasts accepted tx, sends response.
- `Server::on_create_block`: parses candidate block, checks Merkle/target/previous hash, calls `Chain::add_block`, broadcasts accepted block, sends response.
- `Server::send`: writes TCP envelope and payload.
- `Server::serialize_utxo_response`: encodes `(OutPoint, amount)` list.
- `Server::serialize_tx_response`: encodes transaction result response.

## `web.cpp`

- `json_escape`: escapes string content for JSON.
- `json_response`: builds Crow JSON response with CORS headers.
- `error_response`: builds structured JSON error.
- `parse_u32`: strict decimal unsigned 32-bit parser.
- `hex_to_bytes`: decodes even-length hex into bytes.
- `parse_signed_transaction_payload`: reads HTTP raw transaction payload and rejects trailing bytes.
- `tx_error_to_string`: maps transaction errors to text.
- `outpoint_json`: serializes an outpoint object.
- `output_json`: serializes a transaction output object.
- `transaction_json`: serializes full transaction JSON.
- `block_summary_json`: serializes compact block summary JSON.
- `block_json`: serializes full block JSON with txids and transactions.
- `event_new_tx_json`: serializes WebSocket new transaction event.
- `event_new_block_json`: serializes WebSocket new block event.
- `WebServer::WebServer`: stores chain/port and registers routes.
- `WebServer::run`: starts Crow multithreaded app.
- `WebServer::stop`: stops Crow app.
- `WebServer::broadcast_new_tx`: broadcasts transaction event.
- `WebServer::broadcast_new_block`: broadcasts block event.
- `WebServer::broadcast_text`: sends text to all tracked WebSocket connections under mutex.
- `WebServer::setup_routes`: registers HTTP and WebSocket routes.

## `main.cpp`

- `main`: initializes libsodium, constructs `Chain`, `WebServer`, and `Server`, wires accepted tx/block callbacks, runs Crow on a thread and TCP on the main thread, stops web server after TCP run exits.

## Tests

- `test_tx_roundtrip`: verifies transaction serialization/deserialization preserves expected fields and txid.
- `test_block_roundtrip`: verifies block serialization/deserialization preserves hash/header/transaction identity.
- `test_malformed_rejected`: verifies empty transaction bytes throw.
- `test_coinbase`: verifies no-input transaction is coinbase.
- `test_merkle`: verifies two-leaf Merkle root equals Blake2b of concatenated leaves.
- test `main`: initializes libsodium and runs all tests.
