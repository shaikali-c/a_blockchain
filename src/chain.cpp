#include "axis/chain.h"

#include "axis/crypto.h"
#include "axis/util.h"

#include <leveldb/db.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <shared_mutex>
#include <utility>

static std::string hex_key(const Hash& h) {
    std::string hex(h.size() * 2 + 1, '\0');
    sodium_bin2hex(hex.data(), hex.size(), h.data(), h.size());
    hex.pop_back();
    return hex;
}

Chain::Chain() {
    leveldb::Options opts;
    opts.create_if_missing = true;

    leveldb::DB* raw = nullptr;
    auto st = leveldb::DB::Open(opts, "blocks", &raw);
    if (!st.ok())
        throw std::runtime_error("blocks DB: " + st.ToString());
    blocks_db_.reset(raw);

    st = leveldb::DB::Open(opts, "pool", &raw);
    if (!st.ok())
        throw std::runtime_error("pool DB: " + st.ToString());
    pool_db_.reset(raw);

    load_blocks();
    load_pool();
    if (blocks_.empty())
        create_genesis(); // will be added in chain.cpp after class

    logging::info("chain contains " + std::to_string(blocks_.size()) +
                  " block(s)");
    for (size_t i = 0; i < blocks_.size(); ++i) {
        std::cout << "Block #" << i << "\n";
        std::cout << blocks_[i] << "\n";
    }

    dump_utxo();
    build_target();
}

Chain::~Chain() = default;

Block Chain::tip() const {
    std::shared_lock lock(mutex_);
    return blocks_.back();
}

Hash Chain::tip_hash() const {
    std::shared_lock lock(mutex_);
    return blocks_.back().hash();
}

uint32_t Chain::height() const {
    std::shared_lock lock(mutex_);
    return height_;
}

uint8_t Chain::get_difficulty() const {
    std::shared_lock lock(mutex_);
    return difficulty_;
}

Hash Chain::target() const {
    std::shared_lock lock(mutex_);
    return target_;
}

std::optional<Block> Chain::get_block(uint32_t height) const {
    std::shared_lock lock(mutex_);
    if (height >= blocks_.size())
        return std::nullopt;
    return blocks_[height];
}

std::optional<std::pair<uint32_t, Block>> Chain::get_block(const Hash& hash) const {
    std::shared_lock lock(mutex_);
    for (uint32_t i = 0; i < blocks_.size(); ++i) {
        if (blocks_[i].hash() == hash)
            return std::pair{i, blocks_[i]};
    }
    return std::nullopt;
}

std::vector<Block> Chain::get_blocks(uint32_t start, uint32_t count) const {
    std::shared_lock lock(mutex_);
    std::vector<Block> blocks;
    if (start >= blocks_.size() || count == 0)
        return blocks;

    const auto available = static_cast<uint32_t>(blocks_.size() - start);
    const auto end = start + std::min(count, available);
    blocks.reserve(end - start);
    for (uint32_t i = start; i < end; ++i)
        blocks.push_back(blocks_[i]);
    return blocks;
}

void Chain::load_blocks() {
    logging::info("loading blocks from database");

    auto it = std::unique_ptr<leveldb::Iterator>(
        blocks_db_->NewIterator(leveldb::ReadOptions()));
    it->SeekToFirst();

    uint32_t loaded = 0;
    for (; it->Valid(); it->Next()) {
        const std::string key = it->key().ToString();
        const std::string raw = it->value().ToString();

        try {
            Reader reader{raw};
            Block blk = Block::deserialize(reader);

            if (reader.offset != raw.size()) {
                throw std::runtime_error(
                    "trailing bytes after block payload: consumed " +
                    std::to_string(reader.offset) + " of " +
                    std::to_string(raw.size()) + " bytes");
            }

            for (const auto& tx : blk.transactions)
                apply_tx(tx);

            blocks_.push_back(std::move(blk));
            loaded++;
        } catch (const std::exception& ex) {
            throw std::runtime_error(
                "failed to deserialize block at key '" + key + "': " +
                ex.what());
        }
    }

    auto status = it->status();
    if (!status.ok()) {
        throw std::runtime_error(
            "error while iterating blocks DB: " + status.ToString());
    }

    height_ = static_cast<uint32_t>(blocks_.size());
    logging::info("loaded " + std::to_string(loaded) +
                  " block(s) from database");
}

void Chain::load_pool() {
    auto it = std::unique_ptr<leveldb::Iterator>(
        pool_db_->NewIterator(leveldb::ReadOptions()));
    it->SeekToFirst();
    for (; it->Valid(); it->Next()) {
        Transaction tx{it->value().ToString()};
        for (const auto& in : tx.inputs)
            pool_spent_[in] = in;
        pool_[tx.txid()] = std::move(tx);
    }
}

void Chain::apply_tx(const Transaction& tx) {
    for (const auto& in : tx.inputs)
        utxo_.erase(in);
    uint32_t idx = 0;
    for (const auto& out : tx.outputs) {
        utxo_[OutPoint{tx.txid(), idx}] = out;
        idx++;
    }
}

static const Address GENESIS_ADDR = [] {
    return from_hex<20>("f45a20e043b01f65638a46831ce79b8fec3f6737");
}();

void Chain::create_genesis() {
    Hash prev{};
    prev.fill(0);

    std::vector<TxOutput> outs = {{GENESIS_ADDR, 15 * UNITS}};
    Transaction coinbase{{}, std::move(outs), Timestamp{1781545365}};

    Block blk{prev, {std::move(coinbase)}, Timestamp{1781545365}, 31496, difficulty_};
    store_block(blk);
    for (const auto& tx : blk.transactions)
        apply_tx(tx);
    blocks_.push_back(std::move(blk));
    height_ = 1;
}

void Chain::rebuild_utxo() {
    utxo_.clear();
    for (const auto& blk : blocks_)
        for (const auto& tx : blk.transactions)
            apply_tx(tx);
}

void Chain::dump_utxo() const {
    logging::info("UTXO set contains " + std::to_string(utxo_.size()) +
                  " output(s)");

    if (utxo_.empty()) {
        std::cout << "UTXO Set\n";
        std::cout << "└─ (empty)\n";
        return;
    }

    std::vector<std::pair<OutPoint, TxOutput>> entries;
    entries.reserve(utxo_.size());
    for (const auto& [op, output] : utxo_)
        entries.emplace_back(op, output);

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        auto a_txid = to_hex(a.first.txid);
        auto b_txid = to_hex(b.first.txid);
        if (a_txid != b_txid)
            return a_txid < b_txid;
        return a.first.index < b.first.index;
    });

    uint64_t total_amount = 0;
    std::cout << "UTXO Set\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& [op, output] = entries[i];
        const bool last = (i + 1 == entries.size());
        const char* branch = last ? "└─" : "├─";

        total_amount += output.amount;

        std::cout << branch << " OutPoint\n";
        std::cout << "   ├─ TxID:       " << short_hex(op.txid) << "\n";
        std::cout << "   ├─ Index:      " << op.index << "\n";
        std::cout << "   ├─ Recipient:  " << short_addr(output.recipient)
                  << " (" << to_hex(output.recipient) << ")\n";
        std::cout << "   └─ Amount:     " << format_amount(output.amount)
                  << " AXIS\n";
    }

    std::cout << "Total UTXO Balance: " << format_amount(total_amount)
              << " AXIS\n";
}

TxError Chain::add_tx(const SignedTransaction& st) {
    std::unique_lock lock(mutex_);
    const auto& tx = st.tx;
    const auto& pk = st.pubkey;
    const auto& sig = st.sig;

    uint64_t sum_out = 0;
    for (const auto& out : tx.outputs) {
        if (out.amount == 0)
            return TxError::ZeroAmount;
        if (sum_out > std::numeric_limits<uint64_t>::max() - out.amount)
            return TxError::InvalidPayload;
        sum_out += out.amount;
    }
    if (sum_out == 0)
        return TxError::ZeroAmount;

    if (tx.inputs.empty())
        return TxError::InvalidPayload;

    Address sender = derive_address(pk);
    uint64_t sum_in = 0;
    for (const auto& in : tx.inputs) {
        auto it = utxo_.find(in);
        if (it == utxo_.end()) {
            std::cout << "Sum in: " << sum_in << "\nSum out: " << sum_out << "\n";
            return TxError::BadOwnership;
        }
        if (it->second.recipient != sender){
            return TxError::BadOwnership;
        }
        if (sum_in > std::numeric_limits<uint64_t>::max() - it->second.amount)
            return TxError::InvalidPayload;
        sum_in += it->second.amount;
    }
    if (sum_in < sum_out)
        return TxError::BadOwnership;

    if (!verify_sig(pk, tx.txid(), sig))
        return TxError::BadSignature;

    if (pool_.contains(tx.txid()))
        return TxError::Duplicate;

    for (const auto& in : tx.inputs)
        if (pool_spent_.contains(in))
            return TxError::InputSpent;

    for (const auto& in : tx.inputs)
        pool_spent_[in] = in;
    pool_[tx.txid()] = tx;

    auto key = hex_key(tx.txid());
    auto status = pool_db_->Put(leveldb::WriteOptions(), key, tx.serialize());
    if (!status.ok())
        throw std::runtime_error("failed to store pool transaction '" + key +
                                 "': " + status.ToString());

    return TxError::None;
}

void Chain::get_utxos(
    const Address& addr,
    std::vector<std::pair<OutPoint, uint64_t>>& outpoints) const {
    std::shared_lock lock(mutex_);
    for (const auto& [op, output] : utxo_) {
        if (output.recipient == addr && !pool_spent_.contains(op)) {
            outpoints.push_back(std::pair(op, output.amount));
        }
    }
}

void Chain::build_target() {
    target_.fill(0xff);
    for (int i = 0; i < difficulty_; i++)
        target_[i] = 0x00;
}

void Chain::store_block(const Block& blk) {
    auto key = block_key(static_cast<uint32_t>(blocks_.size()));
    auto status = blocks_db_->Put(leveldb::WriteOptions(), key, blk.serialize());
    if (!status.ok()) {
        throw std::runtime_error(
            "failed to store block at key '" + key + "': " +
            status.ToString());
    }
}

std::string Chain::block_key(uint32_t height) {
    char buf[11];
    std::snprintf(buf, sizeof(buf), "%010u", height);
    return {buf, 10};
}

bool Chain::pool_contains(const Hash& txid) const {
    std::shared_lock lock(mutex_);
    return pool_.contains(txid);
}

Transaction Chain::get_pool_tx(const Hash& txid) const {
    std::shared_lock lock(mutex_);
    return pool_.at(txid);
}

std::vector<Transaction> Chain::get_pool_txs() const {
    std::shared_lock lock(mutex_);
    std::vector<Transaction> txs;
    txs.reserve(pool_.size());
    for (const auto& [txid, tx] : pool_)
        txs.push_back(tx);
    return txs;
}

bool Chain::verify_block_header(const Block& blk) const {
    if (blk.header().prev_hash != tip_hash())
        return false;
    if (blk.header().timestamp <= tip().header().timestamp)
        return false;
    Hash block_hash = blk.hash();
    for (int i = 0; i < blk.header().difficulty; i++)
        if (block_hash[i] != 0)
            return false;
    return true;
}

void Chain::add_block(const Block& blk) {
    std::unique_lock lock(mutex_);
    for (const auto& tx : blk.transactions) {
        apply_tx(tx);

        if (tx.is_coinbase()) {
            continue;
        }

        for (const auto& in : tx.inputs) {
            pool_spent_.erase(in);
        }

        pool_.erase(tx.txid());

        auto pool_key = hex_key(tx.txid());
        auto status = pool_db_->Delete(leveldb::WriteOptions(), pool_key);
        if (!status.ok()) {
            throw std::runtime_error(
                "failed to remove mined transaction '" + pool_key +
                "' from pool DB: " + status.ToString());
        }
    }

    store_block(blk);
    blocks_.push_back(blk);
    height_++;
}
