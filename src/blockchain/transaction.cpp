#include "axis/blockchain/transaction.h"
#include "axis/core/common.h"

std::string Input::getUTXOKey() const {
    return toHex(transaction_hash) + ":" + std::to_string(output_index);
}

Input::Input(const std::string& utxoKey) {
	const UTXOKey parsedKey = parseUTXOKey(utxoKey);
	transaction_hash = parsedKey.txHash;
	output_index = parsedKey.index;
}

bool Input::operator==(const Input& in) const {
    return (transaction_hash == in.transaction_hash) && (output_index == in.output_index);
}

std::string Input::serialize() const {
    BytesWriter writer;
    writer.writeBytes(transaction_hash);
    writer.writeValues(output_index);
    return writer.getStringBytes();
}

std::string UTXO::serialize() const {
    BytesWriter writer;
    writer.writeBytes(owner);
    writer.writeValues(coins);
    return writer.getStringBytes();
}

Transaction::Transaction(const std::string& rawBytes) {
    deserializeTransaction(rawBytes);
}

Transaction::Transaction(const Addr& miner, uint64_t c, std::vector<UTXO> o) : receiver(miner), coins(c), outputs(o) {
    sender.fill(0x00);
	computeTransactionHash();
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

Transaction::Transaction(
    const Addr& s,
    const Addr& r,
    uint64_t c,
    std::vector<Input> i,
    std::vector<UTXO> o,
    uint64_t t
) : sender(s), receiver(r), coins(c), inputs(std::move(i)), outputs(std::move(o)), timestamp(t) {
    computeTransactionHash(t);
}

void Transaction::computeTransactionHash() {
    timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    computeTransactionHash(timestamp);
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
	if (inputSize > reader.remaining() / (HashSize + sizeof(uint32_t))) {
		throw std::runtime_error("Invalid transaction input count");
	}
    inputs.reserve(inputSize);

    for (uint32_t i = 0; i < inputSize; i++) {
        inputs.emplace_back(Input::deserialize(reader));
    }

    uint32_t outputSize = reader.readBytes<uint32_t>();
	if (outputSize > reader.remaining() / (AddrSize + sizeof(uint64_t))) {
		throw std::runtime_error("Invalid transaction output count");
	}
    outputs.reserve(outputSize);
    for (uint32_t i = 0; i < outputSize; i++) {
        outputs.emplace_back(UTXO::deserialize(reader));
    }
    coins = reader.readBytes<uint64_t>();
    timestamp = reader.readBytes<uint64_t>();
    computeTransactionHash(timestamp);
}
