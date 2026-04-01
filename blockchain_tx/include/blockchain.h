#pragma once

#include "pch.h"
#include "transaction.h"
#include "databaseManager.h"
#include "common.h"
#include "logger.h"
#include "server.h"
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
	std::pair<uint64_t, std::vector<std::string>> findUTXO(
		const std::array<unsigned char, crypto_generichash_BYTES>& owner, uint64_t amount
	);

	DBManager transactionDB;
	DBManager utxoDB;
	Server& server;
	void setupRoutes();
	void _getUTXO();
	
public:
	static Blockchain& Blockchain::getInstance();
	void _getTransactions();
	void _createTransaction();

	void init(const std::array<unsigned char, crypto_generichash_BYTES>& owner);
	void listTransactions() const;
	void listUTXO() const;
	void startServer();

	bool createTransaction(
		const std::array<unsigned char, crypto_generichash_BYTES>& s,
		const std::array<unsigned char, crypto_generichash_BYTES>& r,
		uint64_t coins
	);
};