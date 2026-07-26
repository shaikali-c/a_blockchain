#pragma once

#include "axis/block.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace leveldb {
class DB;
}

class Chain {
public:
    Chain();
    ~Chain();

    TxError add_tx(const SignedTransaction& st);

    void get_utxos(
        const Address& addr,
        std::vector<std::pair<OutPoint, uint64_t>>& outpoints
    ) const;

    Block tip() const;
    Hash tip_hash() const;
    uint32_t height() const;
    uint8_t get_difficulty() const;
    Hash target() const;

    std::optional<Block> get_block(uint32_t height) const;
    std::optional<std::pair<uint32_t, Block>> get_block(const Hash& hash) const;
    std::vector<Block> get_blocks(uint32_t start, uint32_t count) const;

    std::vector<Transaction> get_pool_txs() const;
    bool pool_contains(const Hash& txid) const;
    Transaction get_pool_tx(const Hash& txid) const;

    void add_block(const Block& blk);
    struct HashHasher {
        size_t operator()(const Hash& h) const noexcept {
            size_t v;
            std::memcpy(&v, h.data(), sizeof(v));
            return v;
        }
    };
    std::unordered_map<Hash, Transaction, HashHasher> pool_;
private:
    mutable std::shared_mutex mutex_;
    std::vector<Block> blocks_;
    uint32_t height_ = 0;
    uint8_t difficulty_ = 3;
    Hash target_{};

    std::unordered_map<OutPoint, TxOutput> utxo_;
    std::unordered_map<OutPoint, OutPoint> pool_spent_;

    std::unique_ptr<leveldb::DB> blocks_db_;
    std::unique_ptr<leveldb::DB> pool_db_;

    static constexpr uint64_t UNITS = 1'000'000;
    static constexpr uint64_t MINER_REWARD = 3 * UNITS;

    void load_blocks();
    void load_pool();
    void rebuild_utxo();
    void dump_utxo() const;
    void apply_tx(const Transaction& tx);
    void build_target();
    void store_block(const Block& blk);

    bool verify_tx(const Transaction& tx, const PublicKey& pk) const;
    bool verify_block(const Block& blk);

    void create_genesis();
    static std::string block_key(uint32_t height);
    bool verify_block_header(const Block& blk) const;
};
