#include "axis/tx.h"

#include "axis/crypto.h"
#include "axis/util.h"

#include <iomanip>
#include <string>
#include <utility>

void Transaction::compute_hash() {
    Writer w;
    for (const auto& in : inputs) {
        w.put_hash(in.txid);
        w.put_u32(in.index);
    }
    for (const auto& out : outputs) {
        w.put_addr(out.recipient);
        w.put_u64(out.amount);
    }
    w.put_u64(timestamp.value);
    txid_ = blake2b(w.buf);
}

Transaction::Transaction(std::vector<OutPoint> ins, std::vector<TxOutput> outs,
                         Timestamp ts)
    : inputs(std::move(ins)), outputs(std::move(outs)), timestamp(ts) {
    compute_hash();
}

Transaction::Transaction(const std::string& serialized) {
    Reader r{serialized};
    *this = deserialize(r);
}

void Transaction::serialize(Writer& w) const {
    w.put_hash(txid_);
    w.put_u64(timestamp.value);
    w.put_u32(static_cast<uint32_t>(inputs.size()));
    for (const auto& in : inputs) {
        w.put_hash(in.txid);
        w.put_u32(in.index);
    }
    w.put_u32(static_cast<uint32_t>(outputs.size()));
    for (const auto& out : outputs) {
        w.put_addr(out.recipient);
        w.put_u64(out.amount);
    }
}

std::string Transaction::serialize() const {
    Writer w;
    serialize(w);
    return std::move(w).str();
}

Transaction Transaction::deserialize(Reader& r) {
    Transaction tx;
    tx.txid_ = r.take_hash();
    tx.timestamp = Timestamp{r.take_u64()};
    uint32_t in_count = r.take_u32();
    tx.inputs.reserve(in_count);
    for (uint32_t i = 0; i < in_count; i++)
        tx.inputs.push_back({r.take_hash(), r.take_u32()});
    uint32_t out_count = r.take_u32();
    tx.outputs.reserve(out_count);
    for (uint32_t i = 0; i < out_count; i++)
        tx.outputs.push_back({r.take_addr(), r.take_u64()});
    return tx;
}

std::ostream& operator<<(std::ostream& os, const OutPoint& op) {
    os << "{ txid: " << short_hex(op.txid)
       << ",  index: " << op.index << " }";
    return os;
}

std::ostream& operator<<(std::ostream& os, const TxOutput& out) {
    os << "\xe2\x86\x92 " << short_addr(out.recipient)
       << "  " << format_amount(out.amount) << " AXIS";
    return os;
}

void Transaction::pretty(std::ostream& os) const {
    auto COL = 18;
    auto AMT = 18;

    auto rule = [&](int indent) {
        os << std::string(indent, ' ');
        for (int i = 0; i < 3 + COL + 1 + AMT; i++)
            os << "\xe2\x94\x80";
        os << "\n";
    };

    os << "  \xe2\x97\x86  ";
    if (is_coinbase())
        os << "Coinbase";
    else
        os << "Transaction";
    os << "  " << short_hex(txid()) << "\n";
    rule(2);

    if (!inputs.empty()) {
        os << "    Inputs  " << inputs.size() << "\n\n";
        for (const auto& in : inputs)
            os << "      \xe2\x97\x8b  " << in << "\n";
        os << "\n";
    }

    if (!outputs.empty()) {
        os << "    Outputs  " << outputs.size() << "\n";
        uint64_t total = 0;
        for (const auto& out : outputs) {
            total += out.amount;
            os << "      \xe2\x86\x92  "
               << std::left << std::setw(COL) << short_addr(out.recipient)
               << " " << std::right << std::setw(AMT) << (format_amount(out.amount) + " AXIS") << "\n";
        }
        rule(6);
        os << "      " << std::left << std::setw(COL) << "Total"
           << " " << std::right << std::setw(AMT) << (format_amount(total) + " AXIS") << "\n\n";
    }

    os << "    \xe2\x96\xb8  " << format_timestamp(timestamp) << "\n";
}

std::ostream& operator<<(std::ostream& os, const Transaction& tx) {
    tx.pretty(os);
    return os;
}
