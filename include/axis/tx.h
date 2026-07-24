#pragma once

#include "axis/types.h"
#include <functional>
#include <ostream>

struct OutPoint {
    Hash txid;
    uint32_t index;

    bool operator==(const OutPoint& o) const = default;
};

template <>
struct std::hash<OutPoint> {
    size_t operator()(const OutPoint& op) const noexcept {
        size_t h;
        std::memcpy(&h, op.txid.data(), sizeof(h));
        return h ^ op.index;
    }
};

struct TxOutput {
    Address recipient;
    uint64_t amount;

    bool operator==(const TxOutput& o) const = default;
};

class Transaction {
    Hash txid_{};
    void compute_hash();

public:
    std::vector<OutPoint> inputs;
    std::vector<TxOutput> outputs;
    uint64_t timestamp{};

    Transaction() = default;
    Transaction(std::vector<OutPoint> ins, std::vector<TxOutput> outs,
                uint64_t ts);
    explicit Transaction(const std::string& serialized);

    const Hash& txid() const { return txid_; }
    bool is_coinbase() const { return inputs.empty(); }

    void serialize(Writer& w) const;
    std::string serialize() const;
    static Transaction deserialize(Reader& r);

    void pretty(std::ostream& os) const;

    bool operator==(const Transaction& o) const { return txid_ == o.txid_; }
};

struct SignedTransaction {
    Transaction tx;
    PublicKey pubkey;
    Signature sig;
};

std::ostream& operator<<(std::ostream& os, const OutPoint& op);
std::ostream& operator<<(std::ostream& os, const TxOutput& out);
std::ostream& operator<<(std::ostream& os, const Transaction& tx);
