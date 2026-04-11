#pragma once
#include "pch.h"
#include <sodium.h>

using TransactionHash = std::array<unsigned char, crypto_generichash_BYTES>;
using SecretKey = std::array<unsigned char, crypto_sign_SECRETKEYBYTES>;
using PublicKey = std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>;
using Hash = std::array<unsigned char, crypto_generichash_BYTES>;
using Signature = std::array<unsigned char, crypto_sign_BYTES>;
using Addr = std::array<unsigned char, 20>;
using Callback = std::function<void(const drogon::HttpResponsePtr&)>;


constexpr size_t TransactionHashSize = crypto_generichash_BYTES;

namespace Common {
	template <std::size_t N>
	std::string toHex(const std::array<unsigned char, N>& arr) {
        std::string hex;
        hex.resize(N * 2 + 1);
        sodium_bin2hex(
            hex.data(),
            hex.size(),
            arr.data(),
            arr.size()
        );
        hex.pop_back();
        return hex;
	}
    template <std::size_t N>
    std::array<unsigned char, N> toBytes(const std::string& hex) {
        if (hex.size() != N * 2) {
            throw std::runtime_error("Invalid hex length");
        }
        std::array<unsigned char, N> bytes;
        size_t bin_len;
        sodium_hex2bin(
            bytes.data(),
            bytes.size(),
            hex.data(),
            hex.size(),
            nullptr,
            &bin_len,
            nullptr
        );
    }
    template <std::size_t N>
    Hash hashBytes(const std::array<unsigned char, N>& bytes) {
        Hash hash{};
        crypto_generichash(hash.data(), hash.size(), bytes.data(), bytes.size(), nullptr, 0);
        return hash;
    }

}