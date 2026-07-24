#pragma once

#include "axis/types.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace logging {
inline void info(const std::string& msg) {
    std::cout << "[INFO] " << msg << "\n";
}
inline void err(const std::string& msg) {
    std::cout << "[ERR] " << msg << "\n";
}
inline void reject(const std::string& msg) {
    std::cout << "[REJ] " << msg << "\n";
}
} // namespace logging

template <size_t N>
std::string to_hex(const std::array<uint8_t, N>& arr) {
    std::string hex(N * 2 + 1, '\0');
    sodium_bin2hex(hex.data(), hex.size(), arr.data(), arr.size());
    hex.pop_back();
    return hex;
}

template <size_t N>
std::array<uint8_t, N> from_hex(const std::string& hex) {
    if (hex.size() != N * 2)
        throw std::runtime_error("invalid hex length");
    std::array<uint8_t, N> bytes;
    size_t bin_len;
    if (sodium_hex2bin(bytes.data(), bytes.size(), hex.data(), hex.size(),
                        nullptr, &bin_len, nullptr) != 0)
        throw std::runtime_error("invalid hex data");
    return bytes;
}

inline std::string short_hex(const Hash& h) {
    auto full = to_hex(h);
    return full.substr(0, 8) + ".." + full.substr(full.size() - 8);
}

inline std::string short_addr(const Address& a) {
    auto full = to_hex(a);
    return full.substr(0, 8) + ".." + full.substr(full.size() - 8);
}

inline std::string format_amount(uint64_t amount, uint64_t units = 1000000) {
    auto whole = amount / units;
    auto frac = amount % units;
    std::ostringstream oss;
    oss << whole << '.' << std::setw(6) << std::setfill('0') << frac;
    return oss.str();
}

inline std::string format_timestamp(uint64_t ts) {
    auto t = static_cast<std::time_t>(ts);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::gmtime(&t));
    return buf;
}
