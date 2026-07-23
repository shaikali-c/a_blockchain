#include "axis/crypto/cryptography.h"

Hash Cryptography::computeMerkleRoot(const std::vector<Hash>& transactions) {
    Hash merkleRoot{};
    if (transactions.empty()) {
        return merkleRoot;
    }
    std::vector<Hash> currentLevel = transactions;
    if (currentLevel.size() % 2 != 0) currentLevel.push_back(transactions.back());
    while (currentLevel.size() > 1) {
        if (currentLevel.size() % 2 != 0) currentLevel.push_back(currentLevel.back());
        std::vector<Hash> nextLevel;
        for (size_t i = 0; i < currentLevel.size(); i += 2) {
            std::array<unsigned char, crypto_generichash_BYTES * 2> combinedHash{};
            std::copy(currentLevel[i].begin(), currentLevel[i].end(), combinedHash.begin());
            std::copy(currentLevel[i + 1].begin(), currentLevel[i + 1].end(), combinedHash.begin() + currentLevel[i].size());
            nextLevel.push_back(hashBytes(combinedHash));
        }
        currentLevel = std::move(nextLevel);
    }
    merkleRoot = currentLevel.front();
    return merkleRoot;
}
