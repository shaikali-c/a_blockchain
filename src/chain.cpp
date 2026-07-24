#include "axis/chain.h"
#include <cstdio>
#include <limits>

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

    build_target();
}

Chain::~Chain() = default;

void Chain::load_blocks() {
    auto it = std::unique_ptr<leveldb::Iterator>(
        blocks_db_->NewIterator(leveldb::ReadOptions()));
    it->SeekToFirst();
    for (; it->Valid(); it->Next()) {
        Block blk{it->value().ToString()};
        std::cout << blk << "\n";
        for (const auto& tx : blk.transactions)
            apply_tx(tx);
        blocks_.push_back(std::move(blk));
    }
    height_ = static_cast<uint32_t>(blocks_.size());
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
    Transaction coinbase{{}, std::move(outs), 1781545365};

    Block blk{prev, {std::move(coinbase)}, 1781545365, 31496};
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

TxError Chain::add_tx(const SignedTransaction& st) {
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
        if (it == utxo_.end())
            return TxError::BadOwnership;
        if (it->second.recipient != sender)
            return TxError::BadOwnership;
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
    pool_db_->Put(leveldb::WriteOptions(), key, tx.serialize());

    return TxError::None;
}

void Chain::get_utxos(const Address& addr, std::vector<OutPoint>& outpoints,
                      uint64_t& total) const {
    for (const auto& [op, output] : utxo_) {
        if (output.recipient == addr) {
            outpoints.push_back(op);
            total += output.amount;
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
    blocks_db_->Put(leveldb::WriteOptions(), key, blk.serialize());
}

std::string Chain::block_key(uint32_t height) {
    char buf[11];
    std::snprintf(buf, sizeof(buf), "%010u", height);
    return {buf, 10};
}
