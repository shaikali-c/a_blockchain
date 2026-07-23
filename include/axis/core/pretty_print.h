#pragma once

#include "pch.h"
#include "axis/blockchain/block.h"
#include "axis/blockchain/transaction.h"
#include "axis/core/common.h"

namespace PrettyPrinter {

[[nodiscard]] std::string formatTimestamp(uint64_t timestamp);
[[nodiscard]] std::string indent(std::string_view text, std::string_view prefix);

[[nodiscard]] std::string toString(const UTXO& utxo, std::string_view prefix = "");
[[nodiscard]] std::string toString(const Input& input, std::string_view prefix = "");
[[nodiscard]] std::string toString(const Transaction& transaction, std::string_view prefix = "");
[[nodiscard]] std::string toString(const SignedTransaction& signedTransaction, std::string_view prefix = "");
[[nodiscard]] std::string toString(const BlockHeader& header, std::string_view prefix = "");
[[nodiscard]] std::string toString(const Block& block, std::string_view prefix = "");

} // namespace PrettyPrinter

std::ostream& operator<<(std::ostream& os, const UTXO& utxo);
std::ostream& operator<<(std::ostream& os, const Input& input);
std::ostream& operator<<(std::ostream& os, const Transaction& transaction);
std::ostream& operator<<(std::ostream& os, const SignedTransaction& signedTransaction);
std::ostream& operator<<(std::ostream& os, const BlockHeader& header);
std::ostream& operator<<(std::ostream& os, const Block& block);
