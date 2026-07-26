#include "axis/block.h"

#include "axis/crypto.h"
#include "axis/util.h"

#include <utility>

Hash BlockHeader::hash() const {
    Writer w;
    serialize(w);
    return blake2b(w.buf);
}

void BlockHeader::serialize(Writer& w) const {
    w.put_hash(prev_hash);
    w.put_hash(merkle_root);
    w.put_u64(timestamp.value);
    w.put_u64(nonce);
}

BlockHeader BlockHeader::deserialize(Reader& r) {
    return {
        r.take_hash(),
        r.take_hash(),
        Timestamp{r.take_u64()},
        r.take_u64(),
    };
}

static Hash compute_block_merkle_root(const std::vector<Transaction>& txs) {
    std::vector<Hash> leaves;
    leaves.reserve(txs.size());
    for (const auto& tx : txs)
        leaves.push_back(tx.txid());
    return compute_merkle_root(leaves);
}

Block::Block(Hash prev, std::vector<Transaction> txs, Timestamp ts, uint64_t nonce)
    : header_{prev, {}, ts, nonce}, transactions(std::move(txs)) {
    header_.merkle_root = compute_block_merkle_root(transactions);
    cached_hash_ = header_.hash();
}

Block::Block(const std::string& serialized) {
    Reader r{serialized};
    *this = deserialize(r);
}

void Block::serialize(Writer& w) const {
    header_.serialize(w);
    w.put_u32(static_cast<uint32_t>(transactions.size()));
    for (const auto& tx : transactions) {
        auto tx_bytes = tx.serialize();
        w.put_u32(static_cast<uint32_t>(tx_bytes.size()));
        w.put_str(tx_bytes);
    }
}

std::string Block::serialize() const {
    Writer w;
    serialize(w);
    return std::move(w).str();
}

Block Block::deserialize(Reader& r) {
    Block blk;
    blk.header_ = BlockHeader::deserialize(r);
    uint32_t tx_count = r.take_u32();
    blk.transactions.reserve(tx_count);
    for (uint32_t i = 0; i < tx_count; i++) {
        uint32_t tx_size = r.take_u32();
        auto tx_data = r.take_view(tx_size);
        Reader tr{tx_data};
        blk.transactions.push_back(Transaction::deserialize(tr));
    }
    blk.cached_hash_ = blk.header_.hash();
    return blk;
}

std::ostream& operator<<(std::ostream& os, const BlockHeader& hdr) {
    os << "\xe2\x94\x9c\xe2\x94\x80 Previous:     " << short_hex(hdr.prev_hash) << "\n";
    os << "\xe2\x94\x9c\xe2\x94\x80 Merkle Root:  " << short_hex(hdr.merkle_root) << "\n";
    os << "\xe2\x94\x9c\xe2\x94\x80 Timestamp:    " << format_timestamp(hdr.timestamp) << "\n";
    os << "\xe2\x94\x9c\xe2\x94\x80 Nonce:        " << hdr.nonce << "\n";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Block& blk) {
    os << short_hex(blk.hash()) << "\n";
    os << "\xe2\x94\x9c\xe2\x94\x80 Previous:     " << short_hex(blk.header().prev_hash) << "\n";
    os << "\xe2\x94\x9c\xe2\x94\x80 Merkle Root:  " << short_hex(blk.header().merkle_root) << "\n";
    os << "\xe2\x94\x9c\xe2\x94\x80 Timestamp:    " << format_timestamp(blk.header().timestamp) << "\n";
    os << "\xe2\x94\x9c\xe2\x94\x80 Nonce:        " << blk.header().nonce << "\n";
    os << "\xe2\x94\x94\xe2\x94\x80 Transactions (" << blk.transactions.size() << ")\n";
    for (size_t i = 0; i < blk.transactions.size(); i++) {
        const auto& tx = blk.transactions[i];
        bool last = (i + 1 == blk.transactions.size());
        os << "   " << (last ? "\xe2\x94\x94" : "\xe2\x94\x9c") << "\xe2\x94\x80 ";
        os << short_hex(tx.txid());
        if (tx.is_coinbase())
            os << "  (coinbase)";
        os << "  [" << tx.inputs.size() << " in,  " << tx.outputs.size() << " out]";
        if (tx.is_coinbase() && !tx.outputs.empty())
            os << "  " << format_amount(tx.outputs[0].amount) << " AXIS";
        os << "\n";
    }
    return os;
}
