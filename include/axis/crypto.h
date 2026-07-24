#pragma once

#include "axis/types.h"
#include <span>

Hash blake2b(std::span<const uint8_t> data);
Hash compute_merkle_root(std::span<const Hash> leaves);
Address derive_address(const PublicKey& pk);
bool verify_sig(const PublicKey& pk, const Hash& msg, const Signature& sig);
Signature sign_msg(const SecretKey& sk, const Hash& msg);
