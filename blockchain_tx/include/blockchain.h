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
	uint64_t height;
	std::vector<Transaction> transactionsPool;

	DBManager transactionDB;
	DBManager utxoDB;
	DBManager blocksDB;

	static constexpr std::string_view LOGS_FOLDER = "logs";
	void updateUTXO(const Transaction& transaction);
	bool verifySignature(const SignedTransaction& st);
	bool verifyTransaction(const Transaction& transaction);
	void loadTransactions();
	void loadBlocks();
	void createGenesisBlock();
	std::string serializeBlock(const Block& block);
	Block deserializeBlock(const std::string&);
	
public:
	static Blockchain& getInstance();
	void addTransaction(const SignedTransaction& signedTransaction);
	void listTransactions() const;
	void listUTXO() const;
	void listBlocks();
	void addTransaction(Transaction& tx);
	void spareCoins(const Addr& owner);

	Hash getCurrentBlockHash() const;
	const std::vector<Transaction>& getTXPool() const;
	
	UTXOResult getUTXO(const Addr& addr, uint64_t coins);

};