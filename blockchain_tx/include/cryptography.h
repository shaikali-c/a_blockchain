#pragma once
#include "common.h"

namespace Cryptography {
	std::string computeMerkleRoot(const std::vector<std::string>& transactions);
}