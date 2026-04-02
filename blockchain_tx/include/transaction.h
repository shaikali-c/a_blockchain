#pragma once
#include <pch.h>
#include <sodium.h>
#include <common.h>

struct UTXO {
    uint64_t coins;
    std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> owner;

    UTXO() : coins(0), owner{} {}
    UTXO(const std::string& data);
    UTXO(uint64_t c, const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& o)
        : coins(c), owner(o) {
    }

    std::string serialize() const;
    void deserialize(const std::string& data);
};

struct Input {
    std::string transaction_hash;
    uint32_t output_index;

    Input() : output_index(0) {}
    Input(const std::string& tx_hash, uint32_t oi)
        : transaction_hash(tx_hash), output_index(oi) {
    }

    std::string getUTXOKey() const;
};

class Transaction {
public:
    std::vector<Input> inputs;
    std::vector<UTXO> outputs;

    std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> sender;
    std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> receiver;
    std::array<unsigned char, crypto_generichash_BYTES> transaction_hash_bytes;

    std::string transaction_hash, transaction_id;
    uint64_t coins;
    uint64_t timestamp;

    Transaction() : coins(0), timestamp(0), receiver{}, sender{}, isCoinbase(false) {}
    Transaction(const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& s,
        const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& r,
        uint64_t c, const std::vector<Input>& inputs, const std::vector<UTXO>& outputs);

    std::string serialize() const;
    bool isCoinbase;
    void deserialize(const std::string& data);
};