# Complete Data Flow

This document follows real execution paths through the current implementation.

## Startup Flow

```mermaid
sequenceDiagram
    participant main as main()
    participant sodium as libsodium
    participant chain as Chain
    participant blocks as blocks LevelDB
    participant pool as pool LevelDB
    participant web as WebServer
    participant tcp as Server

    main->>sodium: sodium_init()
    main->>chain: Chain()
    chain->>blocks: Open("blocks", create_if_missing)
    chain->>pool: Open("pool", create_if_missing)
    chain->>blocks: iterate all key/value entries
    chain->>chain: deserialize blocks and apply transactions to utxo_
    chain->>pool: iterate pending transactions
    chain->>chain: rebuild pool_ and pool_spent_
    alt no blocks loaded
        chain->>chain: create_genesis()
        chain->>blocks: Put block key "0000000000"
        chain->>chain: apply genesis coinbase to utxo_
    end
    chain->>chain: build_target()
    main->>web: WebServer(chain, 8080)
    web->>web: setup_routes()
    main->>tcp: Server(chain, 8889, events)
    main->>web: run() on std::thread
    main->>tcp: run() on main thread
```

## Wallet Creates Transaction via TCP

```mermaid
sequenceDiagram
    participant wallet as TCP wallet
    participant server as Server
    participant chain as Chain
    participant crypto as crypto.cpp
    participant pool as pool LevelDB
    participant web as WebServer

    wallet->>server: packet CreateTransaction
    server->>server: parse_create_tx_payload()
    server->>server: construct Transaction(inputs, outputs, timestamp)
    server->>chain: add_tx(SignedTransaction)
    chain->>chain: reject zero outputs / empty inputs / overflow
    chain->>crypto: derive_address(pubkey)
    chain->>chain: check referenced UTXOs exist and belong to sender
    chain->>chain: check sum_in >= sum_out
    chain->>crypto: verify_sig(pubkey, txid, sig)
    chain->>chain: reject duplicate txid
    chain->>chain: reject inputs already in pool_spent_
    chain->>chain: insert pool_spent_ and pool_
    chain->>pool: Put(hex(txid), tx.serialize())
    chain-->>server: TxError::None or error
    alt accepted
        server->>web: event callback on_tx_accepted(tx)
        web->>web: broadcast new_tx JSON to WebSockets
    end
    server-->>wallet: TransactionResponse
```

## Transaction Submission via HTTP

HTTP `POST /api/transaction` uses the same binary payload layout as TCP `CreateTransaction`, but wrapped as hex in JSON:

```mermaid
sequenceDiagram
    participant client as HTTP client
    participant web as WebServer
    participant chain as Chain
    participant pool as pool LevelDB
    participant ws as WebSocket clients

    client->>web: POST /api/transaction { rawTx: hex }
    web->>web: crow::json::load(body)
    web->>web: max rawTx length check
    web->>web: hex_to_bytes(rawTx)
    web->>web: parse_signed_transaction_payload(bytes)
    web->>chain: add_tx(signed_tx)
    chain->>pool: Put pending tx when accepted
    alt rejected
        web-->>client: JSON error 400
    else accepted
        web->>ws: broadcast new_tx
        web-->>client: { txid, status: submitted }
    end
```

## Miner Creates Block via TCP

```mermaid
sequenceDiagram
    participant miner as TCP miner
    participant server as Server
    participant chain as Chain
    participant blocks as blocks LevelDB
    participant pool as pool LevelDB
    participant web as WebServer

    miner->>server: packet CreateBlock
    server->>server: parse_create_block_payload(payload, chain)
    server->>chain: pool_contains(txid) for each supplied txid
    server->>chain: get_pool_tx(txid) for each supplied txid
    server->>server: construct coinbase Transaction
    server->>server: construct Block(prev_hash, coinbase + pool txs, timestamp, nonce)
    server->>server: compare computed merkle root to wire merkle root
    server->>chain: target()
    server->>server: require block hash <= target
    server->>chain: tip_hash()
    server->>server: require prev_hash == tip_hash
    server->>chain: add_block(block)
    chain->>chain: apply every transaction to utxo_
    chain->>chain: remove mined non-coinbase txs from pool_ and pool_spent_
    chain->>pool: Delete mined tx keys
    chain->>blocks: Put next height key => block.serialize()
    chain->>chain: push block and increment height_
    server->>web: event callback on_block_accepted(block)
    web->>web: broadcast new_block JSON
    server-->>miner: CreateBlockResponse
```

Important: `Chain::add_block()` assumes the block has already been checked by the caller. It mutates UTXO and mempool state before storing/pushing the block.

## UTXO Lookup

```mermaid
flowchart TD
    A[Client supplies address] --> B{TCP or HTTP?}
    B -->|TCP GetUTXOs| C[Server::on_get_utxos]
    B -->|HTTP GET /api/utxos/address| D[Web route]
    C --> E[Chain::get_utxos]
    D --> E
    E --> F[Scan utxo_]
    F --> G[Filter output.recipient == address]
    G --> H[Exclude outpoints in pool_spent_]
    H --> I[Return txid/index/amount]
```

`get_utxos()` deliberately hides outputs already reserved by pending mempool transactions, so wallets do not select an output that another pending local transaction is already spending.

## Persistence and Recovery Flow

- Blocks are persisted immediately when genesis or accepted blocks are stored.
- Pending transactions are persisted immediately when accepted into the mempool.
- On startup, all persisted blocks are loaded and replayed through `apply_tx()` to reconstruct the UTXO set.
- On startup, all persisted pool transactions are loaded into `pool_`, and their inputs are inserted into `pool_spent_`.
- If no blocks are present, genesis is created and persisted.

## Broadcast Flow

TCP accepted transactions and blocks are bridged into WebSocket events via callbacks supplied by `main()`:

```mermaid
flowchart LR
    TCP[Server accepts tx/block] --> Event[ServerEvents callback]
    Event --> Web[WebServer broadcast_new_tx/block]
    Web --> Mutex[lock ws_mutex_]
    Mutex --> Connections[iterate ws_connections_]
    Connections --> Send[send_text JSON]
```

HTTP accepted transactions call `broadcast_new_tx()` directly from the HTTP route.
