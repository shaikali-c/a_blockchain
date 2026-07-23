#pragma once

#include "pch.h"
#include "axis/blockchain/block.h"
#include "axis/blockchain/transaction.h"
#include "axis/core/common.h"
#include "axis/core/logger.h"
#include "axis/crypto/cryptography.h"
#include "axis/storage/database_manager.h"
#include <sodium.h>
#include <expected>
#include <asio.hpp>

class Blockchain {
private:
	struct AddressUtxos {
		std::vector<Input> inputs;
		uint64_t totalCoins{};
	};

	struct TransactionRejection {
		TransactionErrorCode code;
		std::string_view reason;
	};

	Blockchain();
	Blockchain(const Blockchain&) = delete;
	Blockchain& operator=(const Blockchain&) = delete;

	std::vector<Block> blocks;
	uint64_t height;
	uint8_t difficulty;
	Hash target{};

	std::unordered_map<std::string, Transaction> transactionsPool;
	std::unordered_map<std::string, Transaction> transactions;
	std::unordered_map<std::string, Input> mempoolInputs;

	std::unordered_map<std::string, UTXO> utxo;
	std::unordered_map<std::string, size_t> blocksMap;

	DatabaseManager blocksDB;
	DatabaseManager poolsDB;

	static constexpr uint64_t UNITS = 1000000;
	static constexpr uint64_t MINER_REWARD = 3 * UNITS;
	static constexpr uint64_t GENESIS_REWARD = 15 * UNITS;

	bool verifySignature(const SignedTransaction& signedTransaction) const;
	bool verifyCoinbaseTransaction(const Transaction& transaction) const;
	bool verifyDifficulty(const Hash& hash);
	bool verifyInputs(const SignedTransaction& signedTransaction) const;
	bool verifyBlock(const Block& block);
	bool transactionInPool(const std::string&) const;
	[[nodiscard]] std::expected<void, TransactionRejection> acceptTransaction(
		const SignedTransaction& signedTransaction
	);

	void loadBlocks();
	void loadPoolTransactions();

	[[nodiscard]] static Addr decodeAddressRequest(std::span<const unsigned char> payload);
	[[nodiscard]] static SignedTransaction deserializeCreateTransactionRequest(
		std::span<const unsigned char> payload
	);
	[[nodiscard]] AddressUtxos findAddressUtxos(const Addr& address) const;
	[[nodiscard]] static std::vector<unsigned char> serializeUtxosResponse(const AddressUtxos& addressUtxos);
	[[nodiscard]] static std::vector<unsigned char> serializeTransactionResponse(
		bool accepted,
		TransactionErrorCode errorCode,
		std::string_view reason
	);
	asio::awaitable<void> sendPacket(
		PayloadType responseType,
		std::vector<unsigned char> payload,
		const std::shared_ptr<asio::ip::tcp::socket>& socket
	);
	asio::awaitable<void> sendTransactionResponse(
		bool accepted,
		TransactionErrorCode errorCode,
		std::string_view reason,
		const std::shared_ptr<asio::ip::tcp::socket>& socket
	);

	void createGenesisBlock();
	std::string generateBlockKey();

	void updateUTXO(const Transaction& transaction);
	Hash buildTarget();

	std::string serializeBlock(const Block& block);
	Block deserializeBlock(const std::string&);

	void acceptClient();
	asio::awaitable<void> readMessage(std::shared_ptr<asio::ip::tcp::socket> socket);

public:
	void setupConnection();

	asio::awaitable<void> handlePayload(PayloadType, std::span<const unsigned char>, std::shared_ptr<asio::ip::tcp::socket>);
	asio::awaitable<void> handleGetUTXO(std::span<const unsigned char>, std::shared_ptr<asio::ip::tcp::socket>);
	asio::awaitable<void> handleGetBlock(std::span<const unsigned char>, std::shared_ptr<asio::ip::tcp::socket>);
	asio::awaitable<void> handleGetUTXOs(std::span<const unsigned char>, std::shared_ptr<asio::ip::tcp::socket>);
	asio::awaitable<void> handleGetTransaction(std::span<const unsigned char>, std::shared_ptr<asio::ip::tcp::socket>);
	// [publicKey][sender][receiver][amount][timestamp][inputCount][inputs][outputCount][outputs][signature]
	asio::awaitable<void> handleCreateTransaction(std::span<const unsigned char>, std::shared_ptr<asio::ip::tcp::socket>);

	static Blockchain& getInstance();
	std::expected<void, std::string> addTransaction(const SignedTransaction& signedTransaction);
	void startServer();
	Hash getCurrentBlockHash() const;

};
