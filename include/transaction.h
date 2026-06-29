#pragma once
#include <pch.h>
#include <sodium.h>
#include <common.h>

struct UTXO {
    Addr owner;
    uint64_t coins;
    UTXO(const Addr& addr, uint64_t c) : owner(addr), coins(c) {}
    std::string serialize() const;
};

struct Input {
    Hash transaction_hash{};
    uint32_t output_index{};
    Input(const Hash tx_hash, uint32_t oi) : transaction_hash(tx_hash), output_index(oi) {}
    Input(const std::string&);
    Input() = default;
    std::string getUTXOKey() const;
    std::string serialize() const;
    bool operator==(const Input&) const;
};

class Transaction {
private:
    void computeTransactionHash();
    void computeTransactionHash(uint64_t);
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
    Transaction(const Addr& miner, uint64_t c, std::vector<UTXO> outputs);
    Transaction(
        const Addr& s,
        const Addr& r,
        uint64_t c,
        std::vector<Input> inputs,
        std::vector<UTXO> outputs,
        uint64_t
    );
    Transaction(const std::string& rawBytes);

    std::string serializeTransaction() const;
    void deserializeTransaction(const std::string&);

};

struct SignedTransaction {
    Transaction transaction;
    PublicKey publicKey;
    Signature signature;
};