#pragma once

#include "axis/block.h"
#include "axis/crypto.h"
#include "axis/util.h"
#include <leveldb/db.h>
#include <memory>

class Chain {
public:
    Chain();
    ~Chain();

    TxError add_tx(const SignedTransaction& st);

    void get_utxos(const Address& addr,
                   std::vector<OutPoint>& outpoints,
                   uint64_t& total) const;

    const Block& tip() const { return blocks_.back(); }
    const Hash& tip_hash() const { return blocks_.back().hash(); }
    uint32_t height() const { return height_; }
    const Hash& target() const { return target_; }

private:
    std::vector<Block> blocks_;
    uint32_t height_ = 0;
    uint8_t difficulty_ = 3;
    Hash target_{};

    std::unordered_map<OutPoint, TxOutput> utxo_;

    struct HashHasher {
        size_t operator()(const Hash& h) const noexcept {
            size_t v;
            std::memcpy(&v, h.data(), sizeof(v));
            return v;
        }
    };
    std::unordered_map<Hash, Transaction, HashHasher> pool_;
    std::unordered_map<OutPoint, OutPoint> pool_spent_;

    std::unique_ptr<leveldb::DB> blocks_db_;
    std::unique_ptr<leveldb::DB> pool_db_;

    static constexpr uint64_t UNITS = 1'000'000;
    static constexpr uint64_t MINER_REWARD = 3 * UNITS;

    void load_blocks();
    void load_pool();
    void rebuild_utxo();
    void apply_tx(const Transaction& tx);
    void build_target();
    void store_block(const Block& blk);

    bool verify_tx(const Transaction& tx, const PublicKey& pk) const;
    bool verify_block(const Block& blk);

    void create_genesis();
    static std::string block_key(uint32_t height);
};
