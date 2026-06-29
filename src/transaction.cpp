#include "Transaction.h"


std::string Input::getUTXOKey() const {
    return toHex(transaction_hash) + ":" + std::to_string(output_index);
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
        transaction_hash = toBytes < Hash{}.size()>(utxoKey.substr(0, colonPos));
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

Transaction::Transaction(const Addr& miner, uint64_t c, std::vector<UTXO> o) : receiver(miner), coins(c), outputs(o) {
    sender.fill(0x00);
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

    BytesWriter writer;
    writer.writeBytes(sender);
    writer.writeBytes(receiver);

    for (const auto& i : inputs) {
        writer.writeBytes(i.transaction_hash);
        writer.writeValues(i.output_index);
    }

    for (const auto& o : outputs) {
        writer.writeBytes(o.owner);
        writer.writeValues(o.coins);
    }

    writer.writeValues(coins);
    writer.writeValues(timestamp);
    transaction_hash = hashBytesVector(writer.getBuffer());
}

void Transaction::computeTransactionHash(uint64_t t) {
    timestamp = t;

    BytesWriter writer;
    writer.writeBytes(sender);
    writer.writeBytes(receiver);

    for (const auto& i : inputs) {
        writer.writeBytes(i.transaction_hash);
        writer.writeValues(i.output_index);
    }

    for (const auto& o : outputs) {
        writer.writeBytes(o.owner);
        writer.writeValues (o.coins);
    }

    writer.writeValues(coins);
    writer.writeValues(timestamp);
    transaction_hash = hashBytesVector(writer.getBuffer());
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

// [sender][receiver][inputSize][inputs][outputSize][outputs][coins][timestamp]

std::string Transaction::serializeTransaction() const {
    BytesWriter writer;
    writer.writeBytes(sender);
    writer.writeBytes(receiver);

    uint32_t inputSize = static_cast<uint32_t>(inputs.size());
    uint32_t outputSize = static_cast<uint32_t>(outputs.size());

    writer.writeValues(inputSize);

    for (const auto& in : inputs) {
        writer.writeBytes(in.serialize());
    }

    writer.writeValues(outputSize);
    for (const auto& out : outputs) {
        writer.writeBytes(out.serialize());
    }

    writer.writeValues(coins);
    writer.writeValues(timestamp);

    return writer.getStringBytes();
}

void Transaction::deserializeTransaction(const std::string& buffer) {
    BytesReader reader{ buffer };
    sender = reader.readBytes<AddrSize>();
    receiver = reader.readBytes<AddrSize>();
    uint32_t inputSize = reader.readBytes<uint32_t>();

    for (uint32_t i = 0; i < inputSize; i++) {
        Hash transactionHash = reader.readBytes<HashSize>();
        uint32_t output_index = reader.readBytes<uint32_t>();
        inputs.emplace_back(transactionHash, output_index);
    }

    uint32_t outputSize = reader.readBytes<uint32_t>();
    for (uint32_t i = 0; i < outputSize; i++) {
        Addr owner = reader.readBytes<AddrSize>();
        uint64_t coins = reader.readBytes<uint64_t>();
        outputs.emplace_back(owner, coins);
    }
    this->coins = reader.readBytes<uint64_t>();
    this->timestamp = reader.readBytes<uint64_t>();
    computeTransactionHash(timestamp);
}