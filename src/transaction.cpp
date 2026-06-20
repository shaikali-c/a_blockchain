#include "Transaction.h"


std::string Input::getUTXOKey() const {
    return Common::toHex(transaction_hash) + ":" + std::to_string(output_index);
}

bool Input::operator==(const Input& in) const {
    return (transaction_hash == in.transaction_hash) && (output_index == in.output_index);
}


std::string Input::serialize() const {
    std::string raw;
    raw.append(reinterpret_cast<const char*>(transaction_hash.data()), transaction_hash.size());
    raw.append(reinterpret_cast<const char*>(&output_index), sizeof(output_index));
    return raw;
}

Input::Input(const std::string& utxoKey) { // Butt, not every Butt is a Butt :D
    size_t colonPos = utxoKey.find(':');
    if (colonPos != std::string::npos) {
        transaction_hash = Common::toBytes < Hash{}.size()>(utxoKey.substr(0, colonPos));
        output_index = std::stoi(utxoKey.substr(colonPos + 1));
    }
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

Transaction::Transaction(const Addr& miner, uint64_t c, std::vector<UTXO> o) : coins(c), outputs(o) {
    sender.fill(0x00);
    receiver = miner;
    coins = c;
    timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    transaction_hash.fill(0x00);
}

Transaction::Transaction(
    const Addr& s,
    const Addr& r,
    uint64_t c,
    std::vector<Input> i,
    std::vector<UTXO> o
) : sender(s), receiver(r), coins(c), inputs(std::move(i)), outputs(std::move(o)) {
    computeTransactionHash();
}

void Transaction::computeTransactionHash() {

    timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    size_t bytesSize = sender.size() + receiver.size();
    for (const auto& i : inputs) {
        bytesSize += i.transaction_hash.size();
        bytesSize += sizeof(i.output_index);
    }
    for (const auto& o : outputs) {
        bytesSize += o.owner.size();
        bytesSize += sizeof(o.coins);
    }
    bytesSize += sizeof(coins);
    bytesSize += sizeof(timestamp);

    std::vector<unsigned char> bytes(bytesSize);

    size_t offset = 0;
    std::memcpy(bytes.data(), sender.data(), sender.size());
    offset += sender.size();
    std::memcpy(bytes.data() + offset, receiver.data(), receiver.size());
    offset += receiver.size();

    for (const auto& i : inputs) {
        std::memcpy(bytes.data() + offset, i.transaction_hash.data(), i.transaction_hash.size());
        offset += i.transaction_hash.size();
        std::memcpy(bytes.data() + offset, &i.output_index, sizeof(i.output_index));
        offset += sizeof(i.output_index);
    }

    for (const auto& o : outputs) {
        std::memcpy(bytes.data() + offset, o.owner.data(), o.owner.size());
        offset += o.owner.size();
        std::memcpy(bytes.data() + offset, &o.coins, sizeof(o.coins));
        offset += sizeof(o.coins);
    }

    std::memcpy(bytes.data() + offset, &coins, sizeof(coins));
    offset += sizeof(coins);
    std::memcpy(bytes.data() + offset, &timestamp, sizeof(timestamp));;

    transaction_hash = Common::hashBytesVector(bytes);
}

void Transaction::computeTransactionHash(uint64_t t) {
    timestamp = t;

    size_t bytesSize = sender.size() + receiver.size();
    for (const auto& i : inputs) {
        bytesSize += i.transaction_hash.size();
        bytesSize += sizeof(i.output_index);
    }
    for (const auto& o : outputs) {
        bytesSize += o.owner.size();
        bytesSize += sizeof(o.coins);
    }
    bytesSize += sizeof(coins);
    bytesSize += sizeof(timestamp);

    std::vector<unsigned char> bytes(bytesSize);

    size_t offset = 0;
    std::memcpy(bytes.data(), sender.data(), sender.size());
    offset += sender.size();
    std::memcpy(bytes.data() + offset, receiver.data(), receiver.size());
    offset += receiver.size();

    for (const auto& i : inputs) {
        std::memcpy(bytes.data() + offset, i.transaction_hash.data(), i.transaction_hash.size());
        offset += i.transaction_hash.size();
        std::memcpy(bytes.data() + offset, &i.output_index, sizeof(i.output_index));
        offset += sizeof(i.output_index);
    }

    for (const auto& o : outputs) {
        std::memcpy(bytes.data() + offset, o.owner.data(), o.owner.size());
        offset += o.owner.size();
        std::memcpy(bytes.data() + offset, &o.coins, sizeof(o.coins));
        offset += sizeof(o.coins);
    }

    std::memcpy(bytes.data() + offset, &coins, sizeof(coins));
    offset += sizeof(coins);
    std::memcpy(bytes.data() + offset, &timestamp, sizeof(timestamp));
    transaction_hash = Common::hashBytesVector(bytes);
}


Transaction::Transaction(
    const Addr& s,
    const Addr& r,
    uint64_t c,
    std::vector<Input> i,
    std::vector<UTXO> o,
    uint64_t t
) : sender(s), receiver(r), coins(c), inputs(i), outputs(o), timestamp(t) {
    computeTransactionHash(t);
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
        Hash transactionHash{};
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