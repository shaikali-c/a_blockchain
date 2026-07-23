#include "axis/core/pretty_print.h"

namespace {

std::string shortHex(std::string_view value, size_t keep = 12) {
    if (value.size() <= keep * 2) {
        return std::string{value};
    }
    return std::string{value.substr(0, keep)} + "..." + std::string{value.substr(value.size() - keep)};
}

std::string bulletLine(std::string_view prefix, std::string_view label, std::string_view value) {
    std::ostringstream out;
    out << prefix << "├─ " << label << ": " << value << '\n';
    return out.str();
}

std::string lastBulletLine(std::string_view prefix, std::string_view label, std::string_view value) {
    std::ostringstream out;
    out << prefix << "└─ " << label << ": " << value << '\n';
    return out.str();
}

} // namespace

namespace PrettyPrinter {

std::string formatTimestamp(uint64_t timestamp) {
    const std::time_t raw = static_cast<std::time_t>(timestamp);
    std::tm timeInfo{};
#if defined(_WIN32)
    gmtime_s(&timeInfo, &raw);
#else
    gmtime_r(&raw, &timeInfo);
#endif
    std::ostringstream out;
    out << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S UTC") << " (" << timestamp << ")";
    return out.str();
}

std::string indent(std::string_view text, std::string_view prefix) {
    std::ostringstream out;
    bool startOfLine = true;
    for (const char ch : text) {
        if (startOfLine) {
            out << prefix;
            startOfLine = false;
        }
        out << ch;
        if (ch == '\n') {
            startOfLine = true;
        }
    }
    return out.str();
}

std::string toString(const UTXO& utxo, std::string_view prefix) {
    std::ostringstream out;
    out << prefix << "UTXO\n";
    out << bulletLine(prefix, "owner", shortHex(toHex(utxo.owner)));
    out << lastBulletLine(prefix, "coins", std::to_string(utxo.coins));
    return out.str();
}

std::string toString(const Input& input, std::string_view prefix) {
    std::ostringstream out;
    out << prefix << "Input\n";
    out << bulletLine(prefix, "transaction_hash", shortHex(toHex(input.transaction_hash)));
    out << lastBulletLine(prefix, "output_index", std::to_string(input.output_index));
    return out.str();
}

std::string toString(const Transaction& transaction, std::string_view prefix) {
    std::ostringstream out;
    out << prefix << "Transaction\n";
    out << bulletLine(prefix, "hash", toHex(transaction.transaction_hash));
    out << bulletLine(prefix, "sender", toHex(transaction.sender));
    out << bulletLine(prefix, "receiver", toHex(transaction.receiver));
    out << bulletLine(prefix, "coins", std::to_string(transaction.coins));
    out << bulletLine(prefix, "timestamp", formatTimestamp(transaction.timestamp));
    out << bulletLine(prefix, "inputs", std::to_string(transaction.inputs.size()));

    if (transaction.inputs.empty()) {
        out << bulletLine(std::string{prefix} + "│ ", "entries", "<none>");
    } else {
        for (size_t i = 0; i < transaction.inputs.size(); ++i) {
            out << prefix << "│  [" << i << "]\n";
            out << indent(toString(transaction.inputs[i]), std::string{prefix} + "│    ");
        }
    }

    out << bulletLine(prefix, "outputs", std::to_string(transaction.outputs.size()));
    if (transaction.outputs.empty()) {
        out << lastBulletLine(std::string{prefix} + "│ ", "entries", "<none>");
    } else {
        for (size_t i = 0; i < transaction.outputs.size(); ++i) {
            out << prefix << "│  [" << i << "]\n";
            out << indent(toString(transaction.outputs[i]), std::string{prefix} + "│    ");
        }
    }

    return out.str();
}

std::string toString(const SignedTransaction& signedTransaction, std::string_view prefix) {
    std::ostringstream out;
    out << prefix << "SignedTransaction\n";
    out << bulletLine(prefix, "public_key", toHex(signedTransaction.publicKey));
    out << bulletLine(prefix, "signature", shortHex(toHex(signedTransaction.signature), 16));
    out << lastBulletLine(prefix, "transaction", "");
    out << indent(toString(signedTransaction.transaction), std::string{prefix} + "   ");
    return out.str();
}

std::string toString(const BlockHeader& header, std::string_view prefix) {
    std::ostringstream out;
    out << prefix << "BlockHeader\n";
    out << bulletLine(prefix, "previous_hash", toHex(header.previous_hash));
    out << bulletLine(prefix, "hash", toHex(header.hash));
    out << bulletLine(prefix, "merkle_root", toHex(header.merkleRoot));
    out << bulletLine(prefix, "timestamp", formatTimestamp(header.timestamp));
    out << lastBulletLine(prefix, "nonce", std::to_string(header.nonce));
    return out.str();
}

std::string toString(const Block& block, std::string_view prefix) {
    std::ostringstream out;
    out << prefix << "Block\n";
    out << bulletLine(prefix, "transaction_count", std::to_string(block.transactions.size()));
    out << lastBulletLine(prefix, "header", "");
    out << indent(toString(block.blockHeader), std::string{prefix} + "   ");

    if (block.transactions.empty()) {
        out << prefix << "└─ transactions: <none>\n";
        return out.str();
    }

    out << prefix << "└─ transactions\n";
    for (size_t i = 0; i < block.transactions.size(); ++i) {
        out << prefix << "   [" << i << "]\n";
        out << indent(toString(block.transactions[i]), std::string{prefix} + "      ");
    }
    return out.str();
}

} // namespace PrettyPrinter

std::ostream& operator<<(std::ostream& os, const UTXO& utxo) {
    return os << PrettyPrinter::toString(utxo);
}

std::ostream& operator<<(std::ostream& os, const Input& input) {
    return os << PrettyPrinter::toString(input);
}

std::ostream& operator<<(std::ostream& os, const Transaction& transaction) {
    return os << PrettyPrinter::toString(transaction);
}

std::ostream& operator<<(std::ostream& os, const SignedTransaction& signedTransaction) {
    return os << PrettyPrinter::toString(signedTransaction);
}

std::ostream& operator<<(std::ostream& os, const BlockHeader& header) {
    return os << PrettyPrinter::toString(header);
}

std::ostream& operator<<(std::ostream& os, const Block& block) {
    return os << PrettyPrinter::toString(block);
}
