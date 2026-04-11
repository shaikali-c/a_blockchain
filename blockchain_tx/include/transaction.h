#pragma once
#include <pch.h>
#include <sodium.h>
#include <common.h>

struct UTXO {
    uint64_t coins;
    Addr owner;
    UTXO(const Addr& addr, uint64_t c) : coins(c), owner(addr) {}
};

struct Input {
    std::string transaction_hash;
    uint32_t output_index;
    Input(const std::string& tx_hash, uint32_t oi) : transaction_hash(tx_hash), output_index(oi) {}
    std::string getUTXOKey() const;
};

class Transaction {
public:
    std::vector<Input> inputs;
    std::vector<UTXO> outputs;

    Addr sender;
    Addr receiver;
    Hash transaction_hash;

    uint64_t coins;
    uint64_t timestamp;

    Transaction(
        const Addr& s,
        const Addr& r,
        uint64_t c,
        std::vector<Input> inputs,
        std::vector<UTXO> outputs
    );

    bool isCoinbase;
};

struct SignedTransaction {
    Transaction transaction;
    PublicKey publicKey;
    std::array<unsigned char, crypto_sign_BYTES> signature;
};