#pragma once

#include "pch.h"
#include "transaction.h"
#include "databaseManager.h"
#include "common.h"
#include "logger.h"
#include "block.h"
#include <sodium.h>

struct UTXOResult {
	std::vector<Input> inputs;
	uint64_t total = 0;
};

class Blockchain {
private:

	Blockchain();
	Blockchain(const Blockchain&) = delete;
	Blockchain& operator=(const Blockchain&) = delete;

	std::unordered_map<std::string, UTXO> utxo;
	std::unordered_map<std::string, Transaction> transactions;
	std::vector<Block> blocks;
	std::vector<Transaction> transactionsPool;

	DBManager transactionDB;
	DBManager utxoDB;

	static constexpr std::string_view LOGS_FOLDER = "logs";
	void updateUTXO(const Transaction& transaction);
	bool verifySignature(const SignedTransaction& st);
	bool verifyTransaction(const Transaction& transaction);
	
public:
	static Blockchain& getInstance();

	void addTransaction(const SignedTransaction& signedTransaction);
	void listTransactions() const;
	void listUTXO() const;
	void addTransaction(Transaction& tx);
	void spareCoins(const Addr& owner);

	Hash getCurrentBlockHash() const;
	const std::vector<Transaction>& getTXPool() const;
	
	UTXOResult getUTXO(const Addr& addr, uint64_t coins);

};