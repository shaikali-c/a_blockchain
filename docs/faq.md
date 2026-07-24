# FAQ

## General

### What is Axis?

Axis is a minimal blockchain node implemented in C++23. It demonstrates the
core components of a UTXO-based cryptocurrency: blocks, transactions,
cryptographic signatures, Merkle trees, Proof of Work, and a TCP server for
client connections.

### Is Axis production-ready?

No. Axis is an educational/demonstration project. It lacks:
- Peer-to-peer networking
- Block mining (nonce is pre-computed for the genesis block)
- Transaction relay
- Wallet software
- Configurable parameters (port, data directory)
- UTXO index by address (full scan per query)

### What can I do with Axis?

- Run a node that serves UTXO queries and accepts transactions
- Submit transactions and verify they're validated correctly
- Query the UTXO set for a given address
- Retrieve blocks and transactions over TCP
- Study the architecture of a minimal blockchain node

### Does Axis work with Bitcoin?

No. Axis is a separate project with its own wire protocol, its own address
format, and its own block structure. It is not compatible with any existing
cryptocurrency.

## Building

### Why does the build fail with "coroutine not found"?

Your compiler is too old. Axis requires C++23 with coroutine support:
- GCC 14+
- Clang 18+

Check your compiler version:
```bash
g++ --version
```

### Why does the build fail with "libsodium not found"?

Install libsodium:
```bash
# Debian/Ubuntu
sudo apt install libsodium-dev

# Arch
sudo pacman -S libsodium

# macOS
brew install libsodium
```

### Why does the build fail with "leveldb not found"?

Install LevelDB:
```bash
# Debian/Ubuntu
sudo apt install libleveldb-dev

# Arch
sudo pacman -S leveldb

# macOS
brew install leveldb
```

## Running

### The node says "Unable to open database"

Delete the `axis_data/` directory and restart:
```bash
rm -rf axis_data/
./build/axisd
```

This happens if the databases were created by a different version of
LevelDB or if the LOCK file was left behind after a crash.

### How do I stop the node?

Press Ctrl+C. The signal handler cleans up sockets and closes databases.

### Can I change the port?

Not yet. The port is hardcoded to 8080. See `design_decisions.md` for why.

### Where is the blockchain data stored?

In `./axis_data/`:
- `blocks/` — LevelDB database of all blocks
- `pool/` — LevelDB database of pending transactions

### Can I run multiple nodes?

Not usefully. Each node runs independently with its own chain. There is no
peer-to-peer synchronization.

## Transactions

### How do I create a transaction?

Use the `SignedTransaction` type. You need:
1. A previously-unspent OutPoint (from a UTXO query)
2. The private key that owns that UTXO
3. The recipient's address

```cpp
Transaction tx{
    {{previous_txid, 0}},      // inputs (OutPoints)
    {{recipient_addr, 100}},   // outputs (TxOutput)
    time_since_epoch()         // timestamp
};

SignedTransaction stx{
    std::move(tx),
    pubkey,
    sign_msg(private_key, tx.txid())
};
```

### Why can't I spend the genesis UTXO?

The genesis UTXO is owned by address
`f45a20e043b01f65638a46831ce79b8fec3f6737`, which is derived from a public
key whose private key is unknown. This is intentional: the genesis coins
exist to demonstrate the UTXO model but are permanently unspendable.

### What happens to the transaction fee?

Currently, there is no transaction fee mechanism. The difference between
input sum and output sum is simply lost (burned). In a production system,
this would be the miner's fee.

### Can I double-spend?

No. Once a UTXO is spent (either in the mempool or in a confirmed block),
it is removed from the UTXO set. Any subsequent attempt to spend it will
be rejected with `BadOwnership`.

### How are transactions identified?

By their **txid**: the 32-byte Blake2b hash of the transaction data
(inputs + outputs + timestamp). The txid is computed in the `Transaction`
constructor and cannot change.

## Network

### What protocol does Axis use?

A custom binary protocol over TCP. See `docs/packet_protocol.md` for the
full specification.

### Is the connection encrypted?

No. Axis uses no encryption. Network security is based on validation: a
forged packet cannot spend coins because it would fail signature
verification. An eavesdropper can see pending transactions, but cannot
modify them.

### What is the magic number `0xDEADBEEF`?

It's the 4-byte magic value at the start of every packet. It helps identify
Axis traffic and allows ignoring random data on the connection.

## Security

### Are the cryptographic operations safe?

The underlying algorithms (Blake2b, Ed25519) are well-studied and widely
used. libsodium provides constant-time implementations. The signatures are
deterministic (no randomness needed).

### What are the biggest security holes?

1. **No TLS**: An attacker on the network path can see all transactions.
2. **No rate limiting**: A client can flood the node with messages.
3. **No input validation on block timestamps**: An attacker could submit a
   block with a timestamp far in the future or past (though bounds checking
   exists).
4. **Single-threaded**: A CPU-heavy operation blocks all other connections.

### Can an attacker crash the node?

Yes. Any client can open a TCP connection and send data. Malformed data
may cause exceptions that terminate the session coroutine (the connection
is dropped) but not the node. However, memory exhaustion (sending very
large payloads) is not bounded and could crash the node.

## Development

### Can I contribute?

The project is currently a personal demonstration. However, the architecture
is designed to be extended. Good first projects:
- Add a UTXO index by address (`unordered_multimap<Address, OutPoint>`)
- Add a `--port` command-line flag
- Make database writes atomic (use `WriteBatch`)
- Add transaction fee logic
- Implement a mining loop that creates blocks from the mempool

### Why C++23?

Modern C++ features (coroutines, `std::span`, `std::optional`, `std::array`)
make the code safer and more expressive than older C++ standards. Coroutines
eliminate callback complexity from network code.

### Why no C++20 modules?

Compiler support for C++20 modules is still inconsistent across GCC, Clang,
and MSVC. Traditional headers work everywhere with C++23.

### Why is the executable so small?

The debug executable is 134K (test) to 899K (daemon). The release executable
is even smaller. This is because Axis uses minimal dependencies and no
framework bloat.

### Why is there no `main.cpp` in the library?

The library (`libaxis_core.a`) contains all blockchain logic but no entry
point. The daemon (`axisd`) links to the library and provides `main()`.
This separation allows linking the library to test code without pulling in
the daemon's entry point.
