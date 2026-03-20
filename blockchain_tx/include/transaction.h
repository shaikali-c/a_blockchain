#pragma once
#include <pch.h>
#include <sodium.h>
#include <common.h>

struct UTXO {
	uint64_t coins;
	std::array<unsigned char, crypto_box_PUBLICKEYBYTES> owner;

	UTXO() : coins(0), owner{} {}
	UTXO(const std::string& data);

	std::string serialize() const;
	void deserialize(const std::string& data);

	UTXO(uint64_t c,
		const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& o)
		: coins(c), owner(o) {}
};

struct Input {
	std::string transaction_hash;
	uint32_t output_index;
	Input(const std::string& tx_hash, uint32_t oi) : transaction_hash(tx_hash), output_index(oi){}
};

class Transaction {
public:
	std::vector<Input> inputs;
	std::vector<UTXO> outputs;
	std::array<unsigned char, crypto_box_PUBLICKEYBYTES> sender;
	std::array<unsigned char, crypto_box_PUBLICKEYBYTES> receiver;
	std::string transaction_hash, transaction_id;
	uint64_t coins;
	uint64_t timestamp;
	Transaction(const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& s, const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& r, uint64_t c, const std::vector<UTXO>& outputs);
};

