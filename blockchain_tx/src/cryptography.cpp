#include "cryptography.h"

std::string computeMerkleRoot(const std::vector<std::string>& transactions) {
    if (transactions.empty()) {
        return "";
    }

    std::vector<std::string> currentLevel;
    for (const auto& tx : transactions) {
        currentLevel.push_back(_hash(tx));
    }

    while (currentLevel.size() > 1) {
        std::vector<std::string> nextLevel;

        for (size_t i = 0; i < currentLevel.size(); i += 2) {
            if (i + 1 < currentLevel.size()) {
                std::string combined = currentLevel[i] + currentLevel[i + 1];
                nextLevel.push_back(_hash(combined));
            }
            else {
                nextLevel.push_back(_hash(currentLevel[i] + currentLevel[i]));
            }
        }

        currentLevel = std::move(nextLevel);
    }

    return currentLevel[0];
}