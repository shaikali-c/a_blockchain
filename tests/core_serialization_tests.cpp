#include "axis/block.h"
#include "axis/crypto.h"
#include "axis/tx.h"

#include <sodium.h>

#include <array>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void test_tx_roundtrip() {
    Address alice{}, bob{};
    Hash txid{};
    alice.fill(0x11);
    bob.fill(0x22);
    txid.fill(0x33);

    std::vector<OutPoint> ins = {{txid, 7}};
    std::vector<TxOutput> outs = {{bob, 42}, {alice, 3}};
    Transaction original{std::move(ins), std::move(outs), Timestamp{123456789}};

    auto wire = original.serialize();
    Transaction restored{wire};

    assert(restored.txid() == original.txid());
    assert(restored.timestamp == original.timestamp);
    assert(restored.inputs.size() == 1);
    assert(restored.inputs[0].txid == txid);
    assert(restored.inputs[0].index == 7);
    assert(restored.outputs.size() == 2);
    assert(restored.outputs[0].recipient == bob);
    assert(restored.outputs[0].amount == 42);
}

void test_block_roundtrip() {
    Address addr{};
    Hash ph{};
    addr.fill(0x44);
    ph.fill(0x55);

    Transaction coinbase{{}, {{addr, 99}}, Timestamp{123456789}};
    Block original{ph, {std::move(coinbase)}, Timestamp{123456789}, 9, 3};

    auto wire = original.serialize();
    Block restored{wire};

    assert(restored.hash() == original.hash());
    assert(restored.header().prev_hash == original.header().prev_hash);
    assert(restored.header().merkle_root == original.header().merkle_root);
    assert(restored.header().nonce == original.header().nonce);
    assert(restored.header().timestamp == original.header().timestamp);
    assert(restored.header().difficulty == original.header().difficulty);
    assert(restored.transactions.size() == 1);
    assert(restored.transactions[0].txid() ==
           original.transactions[0].txid());
}

void test_malformed_rejected() {
    bool ok = false;
    try {
        Transaction invalid{std::string{}};
    } catch (const std::runtime_error&) {
        ok = true;
    }
    assert(ok);
}

void test_coinbase() {
    Address addr{};
    addr.fill(0xaa);
    Transaction coinbase{{}, {{addr, 100}}, Timestamp{0}};
    assert(coinbase.is_coinbase());
}

void test_merkle() {
    Hash a{}, b{};
    a.fill(0x01);
    b.fill(0x02);

    std::vector<Hash> leaves = {a, b};
    auto root = compute_merkle_root(leaves);

    std::array<uint8_t, 64> pair{};
    std::memcpy(pair.data(), a.data(), 32);
    std::memcpy(pair.data() + 32, b.data(), 32);
    auto expected = blake2b(pair);
    assert(root == expected);
}

} // namespace

int main() {
    if (sodium_init() < 0)
        return 1;
    test_tx_roundtrip();
    test_block_roundtrip();
    test_malformed_rejected();
    test_coinbase();
    test_merkle();
    return 0;
}
