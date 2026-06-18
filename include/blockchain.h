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

	std::unordered_map<std::string, Transaction> transactionsPool;
	std::unordered_map<std::string, UTXO> utxo;
	std::unordered_map<std::string, size_t> blocksMap;
	std::unordered_map<std::string, Transaction> transactions;
	std::unordered_map<std::string, Input> mempoolInputs;


	DBManager blocksDB;
	DBManager poolsDB;

	static constexpr uint64_t UNITS = 1000000;
	static constexpr std::string_view LOGS_FOLDER = "logs";
	static constexpr uint64_t MINER_REWARD = 3 * UNITS;

	bool verifySignature(const SignedTransaction& st);
	bool verifyTransaction(const Transaction& transaction);
	bool transactionInPool(const std::string&) const;
	bool verifyDifficulty(const Hash& hash);

	void updateUTXO(const Transaction& transaction);
	void loadBlocks();
	void loadPoolTransactions();
	void createGenesisBlock();
	std::string generateBlockKey();
	Hash buildTarget();

	crow::response createTransaction(const crow::request& req);
	crow::response createBlock(const crow::request& req);
	crow::response getUTXO(const std::string& address);
	crow::response getTransaction(const std::string& hash);
	crow::response getBlock(const std::string& hash);

	std::string serializeBlock(const Block& block);
	Block deserializeBlock(const std::string&);
	
public:
	static Blockchain& getInstance();
	std::expected<void, std::string> addTransaction(const SignedTransaction& signedTransaction);
	void listTransactions() const;
	void listUTXO() const;
	void listBlocks();
	void startServer();
	void listPoolTransactions() const;

	Hash getCurrentBlockHash() const;
};