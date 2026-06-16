#pragma once

#include "pch.h"
#include "transaction.h"
#include "databaseManager.h"
#include "common.h"
#include "logger.h"
#include "block.h"
#include <sodium.h>
#include <expected>

struct UTXOResult {
	std::vector<Input> inputs;
	uint64_t total = 0;
};

class Blockchain {
private:

	Blockchain();
	Blockchain(const Blockchain&) = delete;
	Blockchain& operator=(const Blockchain&) = delete;

	std::vector<Block> blocks;
	uint64_t height;
	uint8_t difficulty;
	Hash target{};

	DBManager transactionDB;
	DBManager blocksDB;
	DBManager poolsDB;

	static constexpr std::string_view LOGS_FOLDER = "logs";
	static constexpr uint64_t MINER_REWARD = 1;
	void updateUTXO(const Transaction& transaction);
	bool verifySignature(const SignedTransaction& st);
	bool verifyTransaction(const Transaction& transaction);
	bool transactionInPool(const std::string&) const;
	bool verifyDifficulty(const Hash& hash);
	void loadTransactions();
	void loadBlocks();
	void loadPoolTransactions();
	void createGenesisBlock();
	std::string generateBlockKey();
	Hash buildTarget();

	crow::response createTransaction(const crow::request& req);
	crow::response createBlock(const crow::request& req);
	crow::response getUTXO(const std::string& address);

	std::string serializeBlock(const Block& block);
	Block deserializeBlock(const std::string&);
	
public:
	static Blockchain& getInstance();
	std::expected<void, std::string> addTransaction(const SignedTransaction& signedTransaction);
	std::unordered_map<std::string, Transaction> transactionsPool;
	std::unordered_map<std::string, UTXO> utxo;
	std::unordered_map<std::string, Transaction> transactions;
	void listTransactions() const;
	void listUTXO() const;
	void listBlocks();

	void startServer();
	Hash getCurrentBlockHash() const;
	void listPoolTransactions() const;
};