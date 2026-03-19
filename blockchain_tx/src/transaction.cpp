#include <transaction.h>

std::string UTXO::serialize() const {
    std::string result;
    result.reserve(sizeof(coins) + owner.size());

    result.append(reinterpret_cast<const char*>(&coins), sizeof(coins));
    result.append(reinterpret_cast<const char*>(owner.data()), owner.size());

    return result;
}

UTXO::UTXO(const std::string& data) {
    deserialize(data);
}

void UTXO::deserialize(const std::string& data) {

    const size_t expected_size = sizeof(uint64_t) + crypto_box_PUBLICKEYBYTES;
    if (data.size() != expected_size) {
        throw std::runtime_error("Invalid UTXO serialized data size");
    }

    size_t offset = 0;
    std::memcpy(&coins, data.data() + offset, sizeof(coins));
    offset += sizeof(coins);
    std::memcpy(owner.data(), data.data() + offset, owner.size());
}


Transaction::Transaction(
    const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& s,
    const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& r,
    uint64_t c,
    const std::vector<UTXO>& o
) : sender(s), receiver(r), coins(c), outputs(o) {

    std::array<unsigned char, crypto_generichash_BYTES> hash;

    crypto_generichash_state state;
    crypto_generichash_init(&state, NULL, 0, hash.size());

    crypto_generichash_update(&state, sender.data(), sender.size());
    crypto_generichash_update(&state, receiver.data(), receiver.size());

    for (const auto& o : outputs) {
        crypto_generichash_update(&state, o.owner.data(), o.owner.size());
        crypto_generichash_update(&state, reinterpret_cast<const unsigned char*>(&o.coins), sizeof(o.coins));
    }

    crypto_generichash_update(&state,
        reinterpret_cast<const unsigned char*>(&coins),
        sizeof(coins)
    );

    crypto_generichash_final(&state, hash.data(), hash.size());

    transaction_hash = toHex(hash.data(), hash.size());

}