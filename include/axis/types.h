#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sodium.h>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using Hash = std::array<uint8_t, 32>;
using Address = std::array<uint8_t, 20>;
using PublicKey = std::array<uint8_t, crypto_sign_PUBLICKEYBYTES>;
using SecretKey = std::array<uint8_t, crypto_sign_SECRETKEYBYTES>;
using Signature = std::array<uint8_t, crypto_sign_BYTES>;

enum class TxError : uint8_t {
    None,
    InvalidPayload,
    BadPubkey,
    ZeroAmount,
    BadOwnership,
    BadSignature,
    Duplicate,
    InputSpent,
    Internal,
};

enum class BlockError : uint8_t {
    None,
    InvalidHeight,
    BadPreviousHash,
    InvalidBlockHash,
    HighHash,
    TimeTooFar,
    TimeTooOld,
    BadSignature,
    MissingInputs,
    Duplicate,
    InvalidPayload,
    Internal,
};

struct Writer {
    std::vector<uint8_t> buf;

    void put_bytes(const void* data, size_t len) {
        auto* p = static_cast<const uint8_t*>(data);
        buf.insert(buf.end(), p, p + len);
    }

    void put_u8(uint8_t v)  { put_bytes(&v, 1); }
    void put_u16(uint16_t v) { put_bytes(&v, 2); }
    void put_u32(uint32_t v) { put_bytes(&v, 4); }
    void put_u64(uint64_t v) { put_bytes(&v, 8); }
    void put_i32(int32_t v)  { put_bytes(&v, 4); }

    void put_hash(const Hash& h)  { put_bytes(h.data(), h.size()); }
    void put_addr(const Address& a) { put_bytes(a.data(), a.size()); }
    void put_pk(const PublicKey& pk)   { put_bytes(pk.data(), pk.size()); }
    void put_sig(const Signature& sig) { put_bytes(sig.data(), sig.size()); }

    void put_span(std::span<const uint8_t> s) {
        buf.insert(buf.end(), s.begin(), s.end());
    }

    void put_str(std::string_view sv) {
        put_span(std::span<const uint8_t>{
            reinterpret_cast<const uint8_t*>(sv.data()), sv.size()
        });
    }

    std::string str() const& {
        return {buf.begin(), buf.end()};
    }

    std::string str() && {
        return {buf.begin(), buf.end()};
    }
};

struct Reader {
    std::string_view data;
    size_t offset = 0;

    explicit Reader(std::string_view sv) : data(sv) {}

    size_t remain() const { return data.size() - offset; }

    void check(size_t n) const {
        if (n > remain())
            throw std::runtime_error("Reader: unexpected end of data");
    }

    uint8_t  take_u8()  { check(1); return data[offset++]; }
    uint16_t take_u16() { check(2); uint16_t v; std::memcpy(&v, data.data() + offset, 2); offset += 2; return v; }
    uint32_t take_u32() { check(4); uint32_t v; std::memcpy(&v, data.data() + offset, 4); offset += 4; return v; }
    uint64_t take_u64() { check(8); uint64_t v; std::memcpy(&v, data.data() + offset, 8); offset += 8; return v; }

    Hash take_hash() {
        check(32);
        Hash h;
        std::memcpy(h.data(), data.data() + offset, 32);
        offset += 32;
        return h;
    }

    Address take_addr() {
        check(20);
        Address a;
        std::memcpy(a.data(), data.data() + offset, 20);
        offset += 20;
        return a;
    }

    PublicKey take_pk() {
        check(crypto_sign_PUBLICKEYBYTES);
        PublicKey pk;
        std::memcpy(pk.data(), data.data() + offset, crypto_sign_PUBLICKEYBYTES);
        offset += crypto_sign_PUBLICKEYBYTES;
        return pk;
    }

    Signature take_sig() {
        check(crypto_sign_BYTES);
        Signature sig;
        std::memcpy(sig.data(), data.data() + offset, crypto_sign_BYTES);
        offset += crypto_sign_BYTES;
        return sig;
    }

    std::string_view take_view(size_t n) {
        check(n);
        auto sv = data.substr(offset, n);
        offset += n;
        return sv;
    }
};

struct Timestamp {
    uint64_t value{};

    Timestamp() = default;
    explicit Timestamp(uint64_t v) : value(v) {}

    static Timestamp now() {
        return Timestamp{static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        )};
    }

    bool operator<=(const Timestamp& o) const { return value <= o.value; }
    bool operator>=(const Timestamp& o) const { return value >= o.value; }
    bool operator< (const Timestamp& o) const { return value <  o.value; }
    bool operator> (const Timestamp& o) const { return value >  o.value; }
    bool operator==(const Timestamp& o) const { return value == o.value; }
    bool operator!=(const Timestamp& o) const { return value != o.value; }
};
