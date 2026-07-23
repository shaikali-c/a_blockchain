#pragma once
#include "axis/core/common.h"

namespace Cryptography {
	Hash computeMerkleRoot(const std::vector<Hash>& transactions);
}
