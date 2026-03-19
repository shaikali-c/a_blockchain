#pragma once
#include <sodium.h>

template <size_t N>
void printKey(const std::array<unsigned char, N>& container) {
    for (auto b : container)
        printf("%02x", b);
    printf("\n");
}

std::string toHex(const unsigned char* data, size_t size);
std::string _hash(const std::string& s);