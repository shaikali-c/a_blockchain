#include "cryptography.h"

TransactionHash Cryptography::computeMerkleRoot(const std::vector<TransactionHash>& transactions) {
    if (transactions.empty()) {
        return {};
    }

    std::vector<TransactionHash> currentLevel;
    for (const auto& tx : transactions) {
        currentLevel.push_back(tx);
    }

    while (currentLevel.size() > 1) {
        std::vector<TransactionHash> nextLevel;

        for (size_t i = 0; i < currentLevel.size(); i += 2) {
            const auto& left = currentLevel[i];
            const auto& right = (i + 1 < currentLevel.size()) ? currentLevel[i + 1] : currentLevel[i];

            std::array<unsigned char, TransactionHashSize * 2> combined{};
            std::copy(left.begin(), left.end(), combined.begin());
            std::copy(right.begin(), right.end(), combined.begin() + TransactionHashSize);

            nextLevel.emplace_back(Common::hashBytes(combined));
        }

        currentLevel = std::move(nextLevel);
    }

    return currentLevel[0];
}