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

	static constexpr size_t PUBLIC_KEY_BYTES = crypto_box_PUBLICKEYBYTES;
	static constexpr size_t SECRET_KEY_BYTES = crypto_box_SECRETKEYBYTES;

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
	
public:
	static Blockchain& Blockchain::getInstance();

	void init(const std::array<unsigned char, crypto_generichash_BYTES>& owner);
	void listTransactions() const;
	void listUTXO() const;

	bool createTransaction(
		const std::array<unsigned char, crypto_generichash_BYTES>& s,
		const std::array<unsigned char, crypto_generichash_BYTES>& r,
		uint64_t coins
	);
};