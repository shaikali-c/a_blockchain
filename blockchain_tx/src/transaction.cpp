#include "Transaction.h"


std::string Input::getUTXOKey() const {
    return Common::toHex(transaction_hash) + ":" + std::to_string(output_index);
}

std::string Input::serialize() const {
    std::string raw;
    raw.append(reinterpret_cast<const char*>(transaction_hash.data()), transaction_hash.size());
    raw.append(reinterpret_cast<const char*>(&output_index), sizeof(output_index));
    return raw;
}

std::string UTXO::serialize() const {
    std::string raw;
    raw.append(reinterpret_cast<const char*>(owner.data()), owner.size());
    raw.append(reinterpret_cast<const char*>(&coins), sizeof(coins));
    return raw;
}

Transaction::Transaction(const std::string& rawBytes) {
    deserializeTransaction(rawBytes);
}


Transaction::Transaction(
    const Addr& s,
    const Addr& r,
    uint64_t c,
    std::vector<Input> i,
    std::vector<UTXO> o
) : sender(s), receiver(r), coins(c), inputs(std::move(i)), outputs(std::move(o)), isCoinbase(false) {
    computeTransactionHash();
}

void Transaction::computeTransactionHash() {
    timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    Hash hash{};

    crypto_generichash_state state;
    crypto_generichash_init(&state, NULL, 0, hash.size());

    crypto_generichash_update(&state, sender.data(), sender.size());
    crypto_generichash_update(&state, receiver.data(), receiver.size());

    for (const auto& in : inputs) {
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

void Transaction::computeTransactionHash(uint64_t t) {
    timestamp = t;
    Hash hash{};

    crypto_generichash_state state;
    crypto_generichash_init(&state, NULL, 0, hash.size());

    crypto_generichash_update(&state, sender.data(), sender.size());
    crypto_generichash_update(&state, receiver.data(), receiver.size());

    for (const auto& in : inputs) {
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

// [sender][receiver][inputsSize][inputs][outputs][coins][timestamp]

std::string Transaction::serializeTransaction() const {
    std::string rawBuffer;
    rawBuffer.append(reinterpret_cast<const char*>(sender.data()), sender.size());
    rawBuffer.append(reinterpret_cast<const char*>(receiver.data()), receiver.size());

    uint32_t inputSize = static_cast<uint32_t>(inputs.size());
    uint32_t outputSize = static_cast<uint32_t>(outputs.size());

    rawBuffer.append(reinterpret_cast<const char*>(&inputSize), sizeof(inputSize));
    for (const auto& in : inputs) {
        rawBuffer += in.serialize();
    }

    rawBuffer.append(reinterpret_cast<const char*>(&outputSize), sizeof(outputSize));
    for (const auto& out : outputs) {
        rawBuffer += out.serialize();
    }

    rawBuffer.append(reinterpret_cast<const char*>(&coins), sizeof(coins));
    rawBuffer.append(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    return rawBuffer;
}

void Transaction::deserializeTransaction(const std::string& buffer) {
    size_t offset = 0;

    std::memcpy(sender.data(), buffer.data(), sender.size());
    offset += sender.size();

    std::memcpy(receiver.data(), buffer.data() + offset, receiver.size());
    offset += receiver.size();

    uint32_t inputSize = 0;
    std::memcpy(&inputSize, buffer.data() + offset, sizeof(inputSize));
    offset += sizeof(inputSize);

    for (uint32_t i = 0; i < inputSize; i++) {
        TransactionHash transactionHash{};
        uint32_t output_index = 0;

        std::memcpy(transactionHash.data(), buffer.data() + offset, transactionHash.size());
        offset += transactionHash.size();
        std::memcpy(&output_index, buffer.data() + offset, sizeof(output_index));
        offset += sizeof(output_index);
        inputs.emplace_back(transactionHash, output_index);

    }

    uint32_t outputSize = 0;
    std::memcpy(&outputSize, buffer.data() + offset, sizeof(outputSize));
    offset += sizeof(outputSize);

    for (uint32_t i = 0; i < outputSize; i++) {
        Addr owner{};
        uint64_t coins{};

        std::memcpy(owner.data(), buffer.data() + offset, owner.size());
        offset += owner.size();

        std::memcpy(&coins, buffer.data() + offset, sizeof(coins));
        offset += sizeof(coins);

        outputs.emplace_back(owner, coins);
    }

    std::memcpy(&coins, buffer.data() + offset, sizeof(coins));
    offset += sizeof(coins);

    std::memcpy(&timestamp, buffer.data() + offset, sizeof(timestamp));
    computeTransactionHash(timestamp);
}