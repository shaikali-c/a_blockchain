#pragma once
#include "pch.h"
#include <sodium.h>
#include "transaction.h"
#include "databaseManager.h"
#include "common.h"
#include "logger.h"
#include <future>

static constexpr size_t PUBLIC_KEY_BYTES = crypto_box_PUBLICKEYBYTES;
static constexpr size_t SECRET_KEY_BYTES = crypto_box_SECRETKEYBYTES;

class Blockchain {
private:
	Blockchain();
	Blockchain(const Blockchain&) = delete;
	Blockchain& operator=(const Blockchain&) = delete;
public:
	static Blockchain& Blockchain::getInstance() {
		static Blockchain blockchainInstance;
		return blockchainInstance;
	}
	DBManager keysDB;
	DBManager transactionDB;
	DBManager utxoDB;
	std::unordered_map<std::string, UTXO> utxo;
	std::unordered_map<std::string, Transaction> transactions;

	DBManager& getkeysDB();

	void init(const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& owner);

	void loadUTXO();
	void loadTransactions();

	void saveUTXO();

	std::pair<uint64_t, std::vector<std::string>> findUTXO(
		const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& owner, uint64_t amount
	);

	void listTransactions() const;
	void listUTXO() const;

	bool createTransaction(
		const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& s,
		const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& r,
		uint64_t coins
	);
};