#include "axis/blockchain/block.h"

#include <cassert>

namespace {

void verifyTransactionRoundTrip() {
    Addr sender{};
    Addr receiver{};
    Hash inputHash{};
    sender.fill(0x11);
    receiver.fill(0x22);
    inputHash.fill(0x33);

    const Input input{ inputHash, 7 };
    const std::vector<UTXO> outputs{ UTXO{ receiver, 42 }, UTXO{ sender, 3 } };
    const Transaction original{ sender, receiver, 42, { input }, outputs, 123456789 };
    const Transaction restored{ original.serializeTransaction() };

    assert(restored.sender == original.sender);
    assert(restored.receiver == original.receiver);
    assert(restored.transaction_hash == original.transaction_hash);
    assert(restored.timestamp == original.timestamp);
    assert(restored.inputs == original.inputs);
    assert(restored.outputs.size() == original.outputs.size());
    assert(restored.outputs[0].owner == original.outputs[0].owner);
    assert(restored.outputs[0].coins == original.outputs[0].coins);
}

void verifyBlockRoundTrip() {
    Addr address{};
    Hash previousHash{};
    Hash blockHash{};
    address.fill(0x44);
    previousHash.fill(0x55);
    blockHash.fill(0x66);

    const Transaction transaction{ address, 99, { UTXO{ address, 99 } } };
    const Block original{ previousHash, blockHash, 123456789, 9, { transaction } };
    const Block restored{ original.serialize() };

    assert(restored.blockHeader.previous_hash == original.blockHeader.previous_hash);
    assert(restored.blockHeader.hash == original.blockHeader.hash);
    assert(restored.blockHeader.merkleRoot == original.blockHeader.merkleRoot);
    assert(restored.blockHeader.nonce == original.blockHeader.nonce);
    assert(restored.blockHeader.timestamp == original.blockHeader.timestamp);
    assert(restored.transactions.size() == 1);
    assert(restored.transactions.front().transaction_hash == transaction.transaction_hash);
}

void verifyMalformedInputIsRejected() {
    bool rejected = false;
    try {
        [[maybe_unused]] const Transaction invalid{ std::string{} };
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    assert(rejected);
}

} // namespace

int main() {
    assert(sodium_init() >= 0);
    verifyTransactionRoundTrip();
    verifyBlockRoundTrip();
    verifyMalformedInputIsRejected();
}
