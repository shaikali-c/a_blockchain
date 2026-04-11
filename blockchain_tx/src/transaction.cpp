#include "Transaction.h"


std::string Input::getUTXOKey() const {
    return transaction_hash + ":" + std::to_string(output_index);
}

Transaction::Transaction(
    const Addr& s,
    const Addr& r,
    uint64_t c,
    std::vector<Input> i,
    std::vector<UTXO> o
): sender(s), receiver(r), coins(c), inputs(std::move(i)), outputs(std::move(o)), isCoinbase(false) {

    timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    Hash hash{};

    crypto_generichash_state state;
    crypto_generichash_init(&state, NULL, 0, hash.size());

    crypto_generichash_update(&state, sender.data(), sender.size());
    crypto_generichash_update(&state, receiver.data(), receiver.size());

    for (const auto& in: inputs) {
        crypto_generichash_update(&state, reinterpret_cast<const unsigned char*>(in.transaction_hash.data()), in.transaction_hash.size());
        crypto_generichash_update(&state, reinterpret_cast<const unsigned char*>(&in.output_index), sizeof(in.output_index));
    }

    for (const auto& out : outputs) {
        crypto_generichash_update(&state, out.owner.data(), out.owner.size());
        crypto_generichash_update(&state, reinterpret_cast<const unsigned char*>(&out.coins), sizeof(out.coins));
    }

    crypto_generichash_update(&state,
        reinterpret_cast<const unsigned char*>(&coins),
        sizeof(coins)
    );

    crypto_generichash_update(&state, reinterpret_cast<const unsigned char*>(&timestamp), sizeof(timestamp));

    crypto_generichash_final(&state, hash.data(), hash.size());

    transaction_hash = hash;
}