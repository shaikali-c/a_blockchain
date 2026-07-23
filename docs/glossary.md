# Glossary

This glossary defines the technical terms used in the Axis project in plain English.

## Address

A short identifier representing ownership. In Axis, an address is derived by hashing a public key into 20 bytes.

## Asio

A C++ networking and asynchronous I/O library. Axis uses standalone Asio to implement its TCP server.

## Binary serialization

Turning structured data into raw bytes instead of text like JSON.

## Block

A group of transactions plus metadata like previous hash, nonce, timestamp, and Merkle root.

## Blockchain

An ordered chain of blocks where each block references the previous block.

## Block hash

A fixed-size fingerprint identifying a block. In proof-of-work systems, it is also compared against a difficulty target.

## Coinbase transaction

A special transaction that creates block reward coins rather than spending previous outputs. In Axis, a coinbase transaction has no inputs and one output.

## Coroutine

A function that can pause and resume during asynchronous operations. Axis uses C++ coroutines with Asio `awaitable` functions.

## Crow

A C++ web framework. It appears in shared headers for compatibility with some helper templates, but it is not part of the main running TCP path documented here.

## Difficulty

A measure of how hard it is for a block hash to satisfy the proof-of-work rule. In Axis, difficulty controls how many leading target bytes are zeroed.

## Double spend

Trying to spend the same UTXO more than once.

## Ed25519

A digital signature system used by libsodium. Axis uses it for signature verification.

## Endianness

The byte order used to store multi-byte integers in memory. Axis currently depends on native machine layout in several serialized formats.

## Genesis block

The first block in a blockchain. Axis hardcodes its genesis values.

## Hash

A fixed-size fingerprint produced from input data by a hash function.

## Input

A reference to a previous transaction output that a new transaction wants to spend.

## LevelDB

An embedded key/value database. Axis uses it for block and mempool persistence.

## libsodium

A cryptographic library used by Axis for hashing and signature verification.

## Mempool

The set of valid pending transactions that have not yet been included in a block.

## Merkle root

A single hash summarizing all transaction hashes in a block.

## Nonce

A number varied during proof-of-work so a block hash can hopefully satisfy the target.

## Packet

A framed network message containing a payload type and payload bytes.

## Payload

The actual content of a network message, excluding the outer framing fields.

## Peer

Another node in a network. Axis does not yet implement a full peer-to-peer network layer.

## Proof-of-work

A rule requiring a block hash to be small enough relative to a target. It usually requires repeated trial and error over different nonces.

## Public key

The shareable cryptographic key used to verify signatures.

## Private key / Secret key

The secret cryptographic key used to create signatures.

## Serialization

Converting in-memory data structures into bytes for storage or transmission.

## Signature

A cryptographic proof that the holder of a private key authorized a message.

## TCP

A reliable byte-stream transport protocol used by Axis for node communication.

## Transaction

A record describing how value moves from old outputs to new outputs.

## Transaction hash

A hash of the transaction content used as the transaction’s identity.

## UTXO

Unspent Transaction Output. A spendable output created by a previous transaction.

## UTXO set

The collection of all outputs that are currently spendable.

## Wire format

The exact byte layout used when data is sent across a network.
