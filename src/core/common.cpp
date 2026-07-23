#include "axis/core/common.h"

Addr computeAddress(const PublicKey& pk) {
	Addr address{};
	crypto_generichash(address.data(), address.size(), pk.data(), pk.size(), nullptr, 0);
	return address;
}

void appendBytes(std::string& buffer, const void* data, size_t size) {
	buffer.append(reinterpret_cast<const char*>(data), size);
}

Hash hashBytesVector(const std::vector<unsigned char>& bytes) {
	Hash hash{};
	crypto_generichash(hash.data(), hash.size(), bytes.data(), bytes.size(), nullptr, 0);
	return hash;
}


UTXOKey parseUTXOKey(const std::string& key)
{
    constexpr std::size_t HashHexLength = HashSize * 2;

    if (key.size() < HashHexLength + 2) // 64 hex chars + ':' + at least 1 digit
        throw std::runtime_error("Invalid UTXO key");

    if (key[HashHexLength] != ':')
        throw std::runtime_error("Invalid UTXO key");

    UTXOKey result{};

    const std::string hashHex = key.substr(0, HashHexLength);
    result.txHash = toBytes<HashSize>(hashHex);

    result.index = static_cast<uint32_t>(std::stoul(key.substr(HashHexLength + 1)));

    return result;
}
