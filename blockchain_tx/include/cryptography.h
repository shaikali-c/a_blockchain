#pragma once
#include "common.h"

namespace Cryptography {
	Hash computeMerkleRoot(const std::vector<TransactionHash>& transactions);
}