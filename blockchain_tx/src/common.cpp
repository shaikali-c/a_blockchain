#include <common.h>

std::string toHex(const unsigned char* data, size_t size) {
    std::ostringstream oss;
    for (size_t i = 0; i < size; i++)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return oss.str();
}

std::array<unsigned char, crypto_generichash_BYTES> _hashBytes(const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& container) {
    std::array<unsigned char, crypto_generichash_BYTES> hash;
    crypto_generichash(
        hash.data(), hash.size(),
        container.data(), container.size(),
        NULL, 0
    );
    return hash;
}

std::string _hashStr(const std::string& s) {
    std::array<unsigned char, crypto_generichash_BYTES> hash;
    crypto_generichash(
        hash.data(), hash.size(),
        reinterpret_cast<const unsigned char*>(s.data()), s.size(),
        NULL, 0
    );
    return toHex(hash.data(), hash.size());
}