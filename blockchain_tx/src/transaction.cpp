#include "Transaction.h"

static std::string fromHex(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        throw std::runtime_error("Invalid hex string");
    }

    std::string result;
    result.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned char byte = 0;
        for (size_t j = 0; j < 2; ++j) {
            char c = hex[i + j];
            byte <<= 4;
            if (c >= '0' && c <= '9') {
                byte |= c - '0';
            }
            else if (c >= 'a' && c <= 'f') {
                byte |= c - 'a' + 10;
            }
            else if (c >= 'A' && c <= 'F') {
                byte |= c - 'A' + 10;
            }
            else {
                throw std::runtime_error("Invalid hex character");
            }
        }
        result.push_back(static_cast<char>(byte));
    }

    return result;
}

// UTXO implementation
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
    const size_t expected_size = sizeof(uint64_t) + crypto_sign_PUBLICKEYBYTES;
    if (data.size() != expected_size) {
        throw std::runtime_error("Invalid UTXO serialized data size");
    }

    size_t offset = 0;
    std::memcpy(&coins, data.data() + offset, sizeof(coins));
    offset += sizeof(coins);
    std::memcpy(owner.data(), data.data() + offset, owner.size());
}

std::string UTXO::serializeHex() const {
    std::string binary = serialize();
    return toHex(reinterpret_cast<const unsigned char*>(binary.data()), binary.size());
}

void UTXO::deserializeHex(const std::string& hex) {
    deserialize(fromHex(hex));
}

// Transaction implementation
Transaction::Transaction(
    const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& s,
    const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& r,
    uint64_t c,
    const std::vector<UTXO>& o
) : sender(s), receiver(r), coins(c), outputs(o) {

    timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    std::array<unsigned char, crypto_generichash_BYTES> hash;

    crypto_generichash_state state;
    crypto_generichash_init(&state, NULL, 0, hash.size());

    crypto_generichash_update(&state, sender.data(), sender.size());
    crypto_generichash_update(&state, receiver.data(), receiver.size());

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

    transaction_hash = toHex(hash.data(), hash.size());
}

std::string Transaction::serialize() const {
    std::string result;

    // Calculate total size for optimization
    size_t total_size = sender.size() + receiver.size() + sizeof(coins) + sizeof(timestamp);

    // Serialize transaction_hash and transaction_id as length-prefixed strings
    uint32_t hash_size = static_cast<uint32_t>(transaction_hash.size());
    uint32_t id_size = static_cast<uint32_t>(transaction_id.size());
    total_size += sizeof(hash_size) + hash_size + sizeof(id_size) + id_size;

    // Serialize inputs
    uint32_t input_count = static_cast<uint32_t>(inputs.size());
    total_size += sizeof(input_count);
    for (const auto& input : inputs) {
        uint32_t tx_hash_size = static_cast<uint32_t>(input.transaction_hash.size());
        total_size += sizeof(tx_hash_size) + tx_hash_size + sizeof(input.output_index);
    }

    // Serialize outputs
    uint32_t output_count = static_cast<uint32_t>(outputs.size());
    total_size += sizeof(output_count);
    for (const auto& output : outputs) {
        std::string output_data = output.serialize();
        uint32_t output_size = static_cast<uint32_t>(output_data.size());
        total_size += sizeof(output_size) + output_size;
    }

    result.reserve(total_size);

    // Serialize sender public key
    result.append(reinterpret_cast<const char*>(sender.data()), sender.size());

    // Serialize receiver public key
    result.append(reinterpret_cast<const char*>(receiver.data()), receiver.size());

    // Serialize coins
    result.append(reinterpret_cast<const char*>(&coins), sizeof(coins));

    // Serialize timestamp
    result.append(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));

    // Serialize transaction_hash
    uint32_t hash_size_net = static_cast<uint32_t>(transaction_hash.size());
    result.append(reinterpret_cast<const char*>(&hash_size_net), sizeof(hash_size_net));
    if (!transaction_hash.empty()) {
        result.append(transaction_hash.c_str(), hash_size_net);
    }

    // Serialize transaction_id
    uint32_t id_size_net = static_cast<uint32_t>(transaction_id.size());
    result.append(reinterpret_cast<const char*>(&id_size_net), sizeof(id_size_net));
    if (!transaction_id.empty()) {
        result.append(transaction_id.c_str(), id_size_net);
    }

    // Serialize inputs
    uint32_t input_count_net = static_cast<uint32_t>(inputs.size());
    result.append(reinterpret_cast<const char*>(&input_count_net), sizeof(input_count_net));
    for (const auto& input : inputs) {
        uint32_t tx_hash_size = static_cast<uint32_t>(input.transaction_hash.size());
        result.append(reinterpret_cast<const char*>(&tx_hash_size), sizeof(tx_hash_size));
        if (!input.transaction_hash.empty()) {
            result.append(input.transaction_hash.c_str(), tx_hash_size);
        }
        result.append(reinterpret_cast<const char*>(&input.output_index), sizeof(input.output_index));
    }

    // Serialize outputs
    uint32_t output_count_net = static_cast<uint32_t>(outputs.size());
    result.append(reinterpret_cast<const char*>(&output_count_net), sizeof(output_count_net));
    for (const auto& output : outputs) {
        std::string output_data = output.serialize();
        uint32_t output_size = static_cast<uint32_t>(output_data.size());
        result.append(reinterpret_cast<const char*>(&output_size), sizeof(output_size));
        result.append(output_data.c_str(), output_size);
    }

    return result;
}

void Transaction::deserialize(const std::string& data) {
    if (data.empty()) {
        throw std::runtime_error("Empty transaction data");
    }

    size_t offset = 0;

    // Deserialize sender
    if (offset + sender.size() > data.size()) {
        throw std::runtime_error("Invalid transaction data: insufficient data for sender");
    }
    std::memcpy(sender.data(), data.data() + offset, sender.size());
    offset += sender.size();

    // Deserialize receiver
    if (offset + receiver.size() > data.size()) {
        throw std::runtime_error("Invalid transaction data: insufficient data for receiver");
    }
    std::memcpy(receiver.data(), data.data() + offset, receiver.size());
    offset += receiver.size();

    // Deserialize coins
    if (offset + sizeof(coins) > data.size()) {
        throw std::runtime_error("Invalid transaction data: insufficient data for coins");
    }
    std::memcpy(&coins, data.data() + offset, sizeof(coins));
    offset += sizeof(coins);

    // Deserialize timestamp
    if (offset + sizeof(timestamp) > data.size()) {
        throw std::runtime_error("Invalid transaction data: insufficient data for timestamp");
    }
    std::memcpy(&timestamp, data.data() + offset, sizeof(timestamp));
    offset += sizeof(timestamp);

    // Deserialize transaction_hash
    uint32_t hash_size;
    if (offset + sizeof(hash_size) > data.size()) {
        throw std::runtime_error("Invalid transaction data: insufficient data for hash size");
    }
    std::memcpy(&hash_size, data.data() + offset, sizeof(hash_size));
    offset += sizeof(hash_size);

    if (hash_size > 0) {
        if (offset + hash_size > data.size()) {
            throw std::runtime_error("Invalid transaction data: insufficient data for transaction_hash");
        }
        transaction_hash = std::string(data.data() + offset, hash_size);
        offset += hash_size;
    }
    else {
        transaction_hash.clear();
    }

    // Deserialize transaction_id
    uint32_t id_size;
    if (offset + sizeof(id_size) > data.size()) {
        throw std::runtime_error("Invalid transaction data: insufficient data for id size");
    }
    std::memcpy(&id_size, data.data() + offset, sizeof(id_size));
    offset += sizeof(id_size);

    if (id_size > 0) {
        if (offset + id_size > data.size()) {
            throw std::runtime_error("Invalid transaction data: insufficient data for transaction_id");
        }
        transaction_id = std::string(data.data() + offset, id_size);
        offset += id_size;
    }
    else {
        transaction_id.clear();
    }

    // Deserialize inputs
    uint32_t input_count;
    if (offset + sizeof(input_count) > data.size()) {
        throw std::runtime_error("Invalid transaction data: insufficient data for input count");
    }
    std::memcpy(&input_count, data.data() + offset, sizeof(input_count));
    offset += sizeof(input_count);

    inputs.clear();
    inputs.reserve(input_count);
    for (uint32_t i = 0; i < input_count; ++i) {
        uint32_t tx_hash_size;
        if (offset + sizeof(tx_hash_size) > data.size()) {
            throw std::runtime_error("Invalid transaction data: insufficient data for input hash size");
        }
        std::memcpy(&tx_hash_size, data.data() + offset, sizeof(tx_hash_size));
        offset += sizeof(tx_hash_size);

        std::string tx_hash;
        if (tx_hash_size > 0) {
            if (offset + tx_hash_size > data.size()) {
                throw std::runtime_error("Invalid transaction data: insufficient data for input transaction hash");
            }
            tx_hash = std::string(data.data() + offset, tx_hash_size);
            offset += tx_hash_size;
        }

        uint32_t output_index;
        if (offset + sizeof(output_index) > data.size()) {
            throw std::runtime_error("Invalid transaction data: insufficient data for output index");
        }
        std::memcpy(&output_index, data.data() + offset, sizeof(output_index));
        offset += sizeof(output_index);

        inputs.emplace_back(tx_hash, output_index);
    }

    // Deserialize outputs
    uint32_t output_count;
    if (offset + sizeof(output_count) > data.size()) {
        throw std::runtime_error("Invalid transaction data: insufficient data for output count");
    }
    std::memcpy(&output_count, data.data() + offset, sizeof(output_count));
    offset += sizeof(output_count);

    outputs.clear();
    outputs.reserve(output_count);
    for (uint32_t i = 0; i < output_count; ++i) {
        uint32_t output_size;
        if (offset + sizeof(output_size) > data.size()) {
            throw std::runtime_error("Invalid transaction data: insufficient data for output size");
        }
        std::memcpy(&output_size, data.data() + offset, sizeof(output_size));
        offset += sizeof(output_size);

        if (offset + output_size > data.size()) {
            throw std::runtime_error("Invalid transaction data: insufficient data for output data");
        }
        std::string output_data(data.data() + offset, output_size);
        offset += output_size;

        outputs.emplace_back(output_data);
    }
}

std::string Transaction::serializeHex() const {
    std::string binary = serialize();
    return toHex(reinterpret_cast<const unsigned char*>(binary.data()), binary.size());
}

void Transaction::deserializeHex(const std::string& hex) {
    deserialize(fromHex(hex));
}