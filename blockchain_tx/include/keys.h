#pragma once

#include "pch.h"
#include "transaction.h"
#include "common.h"

class KeysManager {
private:
	SecretKey  secretKey;
	PublicKey  publicKey;
	Addr address;
public:
	KeysManager();
	KeysManager(const std::string&);
	~KeysManager() {
		sodium_memzero(secretKey.data(), secretKey.size());
		sodium_memzero(publicKey.data(), publicKey.size());
		sodium_memzero(address.data(), address.size());
	}

	std::string serializeKeys() const;
	Signature signTransaction(const TransactionHash&) const;
	void computeAddr();
	SignedTransaction createTransaction(std::vector<Input> inputs, uint64_t collectedCoins, const Addr& receiver, uint64_t amount) const;
	bool deserializeKeys(const std::string& serializedKeys);
	const Addr& getAddress() const;
};