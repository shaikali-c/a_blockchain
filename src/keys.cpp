#include "keys.h"

KeysManager::KeysManager() {
	crypto_sign_keypair(publicKey.data(), secretKey.data());
	computeAddr();
}

KeysManager::KeysManager(const std::string& serializedKeys) {
	deserializeKeys(serializedKeys);
	computeAddr();
}

std::string KeysManager::serializeKeys() const {
	std::string base;
	base.reserve(secretKey.size() + publicKey.size());
	base.append(reinterpret_cast<const char*>(secretKey.data()), secretKey.size());
	base.append(reinterpret_cast<const char*>(publicKey.data()), publicKey.size());
	return base;
}

bool KeysManager::deserializeKeys(const std::string& serializedKeys) {
	if (serializedKeys.size() != secretKey.size() + publicKey.size()) return 0;
	std::memcpy(secretKey.data(), serializedKeys.data(), secretKey.size());
	std::memcpy(publicKey.data(), serializedKeys.data() + secretKey.size(), publicKey.size());
	computeAddr();
	return true;

}

void KeysManager::computeAddr() {
	crypto_generichash(address.data(), address.size(), publicKey.data(), publicKey.size(), nullptr, 0);
}

Signature KeysManager::signTransaction(const Hash& txHash) const {
	Signature signature{};
	crypto_sign_detached(signature.data(), nullptr, txHash.data(), txHash.size(), secretKey.data());
	return signature;
}

const Addr& KeysManager::getAddress() const {
	return address;
}

SignedTransaction KeysManager::createTransaction(
	std::vector<Input> inputs,
	uint64_t collectedCoins,
	const Addr& receiver,
	uint64_t amount
) const {
	std::vector<UTXO> outputs;
	outputs.reserve(2);
	outputs.emplace_back(receiver, amount);
	if (collectedCoins > amount) 
		outputs.emplace_back(address, collectedCoins - amount);
	Transaction transaction{ address, receiver, amount, std::move(inputs), std::move(outputs) };
	return { transaction, publicKey, signTransaction(transaction.transaction_hash)};
}