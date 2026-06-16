#pragma once
#include "pch.h"
#include <sodium.h>

using SecretKey = std::array<unsigned char, crypto_sign_SECRETKEYBYTES>;
using PublicKey = std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>;
using Hash = std::array<unsigned char, crypto_generichash_BYTES>;
using Signature = std::array<unsigned char, crypto_sign_BYTES>;
using Addr = std::array<unsigned char, 20>;


constexpr size_t TransactionHashSize = crypto_generichash_BYTES;
constexpr size_t TransactionHashHexSize = crypto_generichash_BYTES * 2;

struct BytesWriter {
    std::vector<unsigned char> buffer;

    template<typename T>
    void writeBytes(const T& bytes) {
        buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    }

    template<typename T>
    void writeValues(T value) {
        auto* bytes = reinterpret_cast<unsigned char*>(&value);
        for (size_t i = 0; i < sizeof(value); i++) buffer.push_back(bytes[i]);
    }
    std::vector<unsigned char> getBuffer() const {
        return buffer;
    }
    std::string getStringBytes() const {
        return std::string(buffer.begin(), buffer.end());
    }
};

struct BytesReader {
    std::string rawBytes;
    size_t offset = 0;
    explicit BytesReader(const std::string& b) : rawBytes(b) {}
    template <typename T>
    T readBytes() {
        T value{};
        std::memcpy(&value, rawBytes.data() + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }
    std::string readBytesString(size_t size) {
        std::string result(rawBytes.data() + offset, size);
        offset += size;
        return result;
    }
};

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
        return bytes;
    }
    template <std::size_t N>
    Hash hashBytes(const std::array<unsigned char, N>& bytes) {
        Hash hash{};
        crypto_generichash(hash.data(), hash.size(), bytes.data(), bytes.size(), nullptr, 0);
        return hash;
    }
    Hash hashBytesVector(const std::vector<unsigned char>& bytes);
    Addr computeAddress(const PublicKey& pk);
    void appendBytes(std::string& buffer, const void* data, size_t size);

}

template<std::size_t N>
std::optional<std::array<unsigned char, N>> getBytes(
    const nlohmann::json& json,
    const std::string& key,
    nlohmann::json& errorJson,
    crow::response& res)
{
    if (!json.contains(key)) {
        errorJson["error"] = "Missing field: " + key;
        res.code = 400;
        res.body = errorJson.dump();
        return std::nullopt;
    }
    return Common::toBytes<N>(json[key].get<std::string>());
}

template<typename T>
std::optional<T> getField(
    const nlohmann::json& json,
    const std::string& key,
    nlohmann::json& errorJson,
    crow::response& res)
{
    if (!json.contains(key)) {
        errorJson["error"] = "Missing field: " + key;
        res.code = 400;
        res.body = errorJson.dump();
        return std::nullopt;
    }
    return json[key].get<T>();
}