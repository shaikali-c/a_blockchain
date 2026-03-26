#pragma once
#include <sodium.h>

template <size_t N>
void printKey(const std::array<unsigned char, N>& container) {
    for (const auto b : container)
        printf("%02x", b);
    printf("\n");
}

std::string toHex(const unsigned char* data, size_t size);
std::array<unsigned char, crypto_generichash_BYTES> _hashBytes(const std::string& s);
std::array<unsigned char, crypto_generichash_BYTES> _hashBytes(const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& container);
std::string _hashStr(const std::string& s);

