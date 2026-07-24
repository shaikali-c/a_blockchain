#pragma once

#include "axis/tx.h"
#include <ostream>

struct BlockHeader {
    Hash prev_hash;
    Hash merkle_root;
    uint64_t timestamp;
    uint32_t nonce;
    uint32_t version = 1;

    Hash hash() const;
    void serialize(Writer& w) const;
    static BlockHeader deserialize(Reader& r);
};

class Block {
    BlockHeader header_;
    Hash cached_hash_{};

public:
    std::vector<Transaction> transactions;

    Block() = default;
    Block(Hash prev, std::vector<Transaction> txs, uint64_t ts, uint32_t nonce);
    explicit Block(const std::string& serialized);

    const BlockHeader& header() const { return header_; }
    const Hash& hash() const { return cached_hash_; }

    void serialize(Writer& w) const;
    std::string serialize() const;
    static Block deserialize(Reader& r);
};

std::ostream& operator<<(std::ostream& os, const BlockHeader& hdr);
std::ostream& operator<<(std::ostream& os, const Block& blk);
