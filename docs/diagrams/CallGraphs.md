# Call Graphs

These diagrams show important runtime call paths. They are simplified from the indexed code graph and source inspection.

## Startup Call Graph

```mermaid
graph TD
    main --> sodium_init
    main --> ChainCtor[Chain::Chain]
    ChainCtor --> OpenBlocks[LevelDB Open blocks]
    ChainCtor --> OpenPool[LevelDB Open pool]
    ChainCtor --> load_blocks
    ChainCtor --> load_pool
    ChainCtor --> create_genesis
    ChainCtor --> dump_utxo
    ChainCtor --> build_target
    load_blocks --> BlockDeserialize[Block::deserialize]
    load_blocks --> apply_tx
    create_genesis --> TransactionCtor[Transaction::Transaction]
    create_genesis --> BlockCtor[Block::Block]
    create_genesis --> store_block
    create_genesis --> apply_tx
    main --> WebCtor[WebServer::WebServer]
    WebCtor --> setup_routes
    main --> ServerCtor[Server::Server]
    main --> WebRun[WebServer::run]
    main --> ServerRun[Server::run]
    ServerRun --> do_accept
```

## TCP Transaction Submission Call Graph

```mermaid
graph TD
    handle_client --> handle_msg
    handle_msg --> on_create_tx
    on_create_tx --> parse_create_tx_payload
    parse_create_tx_payload --> ReaderTake[Reader::take_*]
    on_create_tx --> TransactionCtor[Transaction::Transaction]
    TransactionCtor --> compute_hash
    compute_hash --> WriterPut[Writer::put_*]
    compute_hash --> blake2b
    on_create_tx --> add_tx[Chain::add_tx]
    add_tx --> derive_address
    add_tx --> verify_sig
    add_tx --> TransactionSerialize[Transaction::serialize]
    add_tx --> PoolPut[pool_db_->Put]
    on_create_tx --> EventCallback[ServerEvents::on_tx_accepted]
    on_create_tx --> serialize_tx_response
    on_create_tx --> send
```

## TCP Block Submission Call Graph

```mermaid
graph TD
    handle_client --> handle_msg
    handle_msg --> on_create_block
    on_create_block --> parse_create_block_payload
    parse_create_block_payload --> ReaderTake[Reader::take_*]
    parse_create_block_payload --> pool_contains
    parse_create_block_payload --> get_pool_tx
    on_create_block --> BlockCtor[Block::Block]
    BlockCtor --> compute_block_merkle_root
    compute_block_merkle_root --> compute_merkle_root
    BlockCtor --> HeaderHash[BlockHeader::hash]
    HeaderHash --> HeaderSerialize[BlockHeader::serialize]
    HeaderHash --> blake2b
    on_create_block --> target
    on_create_block --> tip_hash
    on_create_block --> add_block[Chain::add_block]
    add_block --> apply_tx
    add_block --> PoolDelete[pool_db_->Delete]
    add_block --> store_block
    store_block --> BlockSerialize[Block::serialize]
    store_block --> BlocksPut[blocks_db_->Put]
    on_create_block --> EventCallback[ServerEvents::on_block_accepted]
    on_create_block --> serialize_block_response
    on_create_block --> send
```

## HTTP Route Call Graph

```mermaid
graph TD
    setup_routes --> StatusRoute[/api/status]
    setup_routes --> TipRoute[/api/tip]
    setup_routes --> BlockRoute[/api/block/id]
    setup_routes --> BlocksRoute[/api/blocks]
    setup_routes --> MempoolRoute[/api/mempool]
    setup_routes --> UTXORoute[/api/utxos/address]
    setup_routes --> TxRoute[/api/transaction]
    setup_routes --> WebSocketRoute[/ws/events]

    StatusRoute --> height
    StatusRoute --> get_difficulty
    StatusRoute --> tip_hash
    TipRoute --> tip
    TipRoute --> block_json
    BlockRoute --> parse_u32
    BlockRoute --> get_block
    BlockRoute --> from_hex
    BlocksRoute --> get_blocks
    BlocksRoute --> block_summary_json
    MempoolRoute --> get_pool_txs
    MempoolRoute --> transaction_json
    UTXORoute --> from_hex
    UTXORoute --> get_utxos
    TxRoute --> hex_to_bytes
    TxRoute --> parse_signed_transaction_payload
    TxRoute --> add_tx
    TxRoute --> broadcast_new_tx
```

## Serialization Call Graph

```mermaid
graph TD
    TransactionSerialize[Transaction::serialize] --> WriterPut[Writer::put_hash/u64/u32/addr]
    TransactionDeserialize[Transaction::deserialize] --> ReaderTake[Reader::take_hash/u64/u32/addr]
    BlockSerialize[Block::serialize] --> HeaderSerialize[BlockHeader::serialize]
    BlockSerialize --> TransactionSerialize
    BlockDeserialize[Block::deserialize] --> HeaderDeserialize[BlockHeader::deserialize]
    BlockDeserialize --> TransactionDeserialize
    HeaderHash[BlockHeader::hash] --> HeaderSerialize
    HeaderHash --> blake2b
```
