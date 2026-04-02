#pragma once
#include <sodium.h>

void toBytes(const std::string& hex, unsigned char* data, size_t size);

std::string toHex(const unsigned char* data, size_t size);
std::string _hashStr(const std::string& s);

std::array<unsigned char, crypto_generichash_BYTES> _hashBytes(const std::string& s);
std::array<unsigned char, crypto_generichash_BYTES> _hashBytes(const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& container);
std::vector<unsigned char> base64_decode(const std::string& base64_string);