#pragma once

#include "pch.h"
#include "transaction.h"
#include "databaseManager.h"
#include "common.h"
#include "logger.h"
#include <future>
#include <sodium.h>

class Blockchain {
private:

	static constexpr size_t PUBLIC_KEY_BYTES = crypto_sign_PUBLICKEYBYTES;

	Blockchain();
	Blockchain(const Blockchain&) = delete;
	Blockchain& operator=(const Blockchain&) = delete;

	void saveUTXO();
	void loadUTXO();
	void loadTransactions();

	std::unordered_map<std::string, UTXO> utxo;
	std::unordered_map<std::string, Transaction> transactions;

	DBManager transactionDB;
	DBManager utxoDB;

	static constexpr std::string_view LOGS_FOLDER = "logs";
	
public:
	static Blockchain& getInstance();

	void init(const std::array<unsigned char, crypto_generichash_BYTES>& owner);
	void listTransactions() const;
	void listUTXO() const;
	void addTransaction(Transaction& tx);
	
	bool verifyTX(Transaction& tx, const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& publicKey, const std::array<unsigned char, crypto_sign_BYTES>& signature);
	bool verifySignature(const std::array<unsigned char, crypto_sign_BYTES>& signature, const std::array<unsigned char, crypto_generichash_BYTES>& msg, const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& publicKey);

	std::pair<std::vector<Input>, uint64_t> getUTXO(const std::array<unsigned char, crypto_generichash_BYTES>& addr, uint64_t coins);

};