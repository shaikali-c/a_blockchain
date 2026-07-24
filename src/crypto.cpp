#include "axis/crypto.h"

Hash blake2b(std::span<const uint8_t> data) {
    Hash h{};
    crypto_generichash(h.data(), h.size(), data.data(), data.size(), nullptr, 0);
    return h;
}

static Hash combine_hash(const Hash& a, const Hash& b) {
    std::array<uint8_t, 64> pair{};
    std::memcpy(pair.data(), a.data(), 32);
    std::memcpy(pair.data() + 32, b.data(), 32);
    return blake2b(pair);
}

Hash compute_merkle_root(std::span<const Hash> leaves) {
    if (leaves.empty())
        return Hash{};
    std::vector<Hash> cur(leaves.begin(), leaves.end());
    if (cur.size() % 2)
        cur.push_back(cur.back());
    while (cur.size() > 1) {
        if (cur.size() % 2)
            cur.push_back(cur.back());
        std::vector<Hash> next;
        next.reserve(cur.size() / 2);
        for (size_t i = 0; i < cur.size(); i += 2)
            next.push_back(combine_hash(cur[i], cur[i + 1]));
        cur = std::move(next);
    }
    return cur[0];
}

Address derive_address(const PublicKey& pk) {
    Address addr{};
    crypto_generichash(addr.data(), addr.size(), pk.data(), pk.size(), nullptr, 0);
    return addr;
}

bool verify_sig(const PublicKey& pk, const Hash& msg, const Signature& sig) {
    return crypto_sign_verify_detached(sig.data(), msg.data(), msg.size(),
                                       pk.data()) == 0;
}

Signature sign_msg(const SecretKey& sk, const Hash& msg) {
    Signature sig{};
    crypto_sign_detached(sig.data(), nullptr, msg.data(), msg.size(), sk.data());
    return sig;
}
