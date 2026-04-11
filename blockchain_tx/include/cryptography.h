#pragma once
#include "common.h"

namespace Cryptography {
	TransactionHash computeMerkleRoot(const std::vector<TransactionHash>& transactions);
}