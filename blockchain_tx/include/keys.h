#pragma once
#include <pch.h>
#include <sodium.h>
#include <common.h>
#include "transaction.h"

class Keys {
private:
	std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> _publicKey;
	std::array<unsigned char, crypto_sign_SECRETKEYBYTES> _secretKey;
	static constexpr size_t ADDR_SIZE = crypto_generichash_BYTES;

public:
	Keys(const std::string& kName);
	Keys();

	void createKeys();
	void setOwner(const std::string&);
	void deserializeKeys(const std::string&);

	std::string _pHex;
	std::string owner;
	std::string serializeKeys() const;

	const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& publicKey() const;

	std::pair<Transaction, std::array<unsigned char, crypto_sign_BYTES>> createTransaction(const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& receiver, uint64_t amount, std::vector<Input> utxo_keys, uint64_t);

	std::array<unsigned char, crypto_sign_BYTES> sign(const unsigned char* data, size_t len);
	std::array<unsigned char, ADDR_SIZE> addr;
};