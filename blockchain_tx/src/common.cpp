#include <common.h>

std::string toHex(const unsigned char* data, size_t size) {
    std::ostringstream oss;
    for (size_t i = 0; i < size; i++)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return oss.str();
}

void toBytes(const std::string & hex, unsigned char* data, size_t size) {

    size_t bin_len;
    const char* hex_end;

    if (sodium_hex2bin(data, size,
        hex.c_str(), hex.length(),
        nullptr,
        &bin_len, &hex_end) != 0) {
        throw std::runtime_error("Failed to convert hex to binary");
    }
}

std::array<unsigned char, crypto_generichash_BYTES> _hashBytes(const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& container) {
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

std::vector<unsigned char> base64_decode(const std::string& base64_string) {
    size_t max_decoded_len = base64_string.length() * 3 / 4;
    std::vector<unsigned char> decoded(max_decoded_len);

    size_t decoded_len;
    if (sodium_base642bin(decoded.data(), max_decoded_len,
        base64_string.c_str(), base64_string.length(),
        nullptr, &decoded_len, nullptr,
        sodium_base64_VARIANT_ORIGINAL) != 0) {
        throw std::runtime_error("Failed to decode base64");
    }

    decoded.resize(decoded_len);
    return decoded;
}