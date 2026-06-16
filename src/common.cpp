#include <common.h>

Addr Common::computeAddress(const PublicKey& pk) {
	Addr address{};
	crypto_generichash(address.data(), address.size(), pk.data(), pk.size(), nullptr, 0);
	return address;
}

void Common::appendBytes(std::string& buffer, const void* data, size_t size) {
	buffer.append(reinterpret_cast<const char*>(data), size);
}

Hash Common::hashBytesVector(const std::vector<unsigned char>& bytes) {
	Hash hash{};
	crypto_generichash(hash.data(), hash.size(), bytes.data(), bytes.size(), nullptr, 0);
	return hash;
}