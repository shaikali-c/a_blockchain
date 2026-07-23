#include "axis/blockchain/block.h"

Block::Block(
	const Hash& ph,
	const Hash& bh,
	uint64_t t,
	uint64_t n,
	const std::vector<Transaction>& txs
) : blockHeader{ ph, bh, t, n }, transactions(txs) {
	std::vector<Hash> tx_hashes;
	tx_hashes.reserve(txs.size());
	for (const auto& transaction : txs) {
		tx_hashes.push_back(transaction.transaction_hash);
	}
	blockHeader.merkleRoot = Cryptography::computeMerkleRoot(tx_hashes);
}

Block::Block(std::string_view rawBytes) {
	// [previousHash][hash][merkleRoot][nonce][timestamp]
	BytesReader reader{ rawBytes };
	Hash previousHash = reader.readBytes < HashSize > ();
	Hash hash = reader.readBytes<HashSize>();
	Hash merkleRoot = reader.readBytes<HashSize>();
	uint64_t nonce = reader.readBytes<uint64_t>();
	uint64_t timestamp = reader.readBytes<uint64_t>();
	size_t transactionsCount = reader.readBytes<size_t>();
	if (transactionsCount > reader.remaining() / sizeof(size_t)) {
		throw std::runtime_error("Invalid block transaction count");
	}
	std::vector<Transaction> txs;

	for (size_t i = 0; i < transactionsCount; i++) {
		size_t transactionDataSize = reader.readBytes<size_t>();
		std::string transactionBytes = reader.readBytesString(transactionDataSize);
		txs.emplace_back(transactionBytes);
	}

	blockHeader = { previousHash, hash, timestamp, nonce, merkleRoot };
	transactions = std::move(txs);
}

std::string Block::serialize() const {
	BytesWriter writer;
	writer.writeBytes(blockHeader.previous_hash);
	writer.writeBytes(blockHeader.hash);
	writer.writeBytes(blockHeader.merkleRoot);
	writer.writeValues(blockHeader.nonce);
	writer.writeValues(blockHeader.timestamp);
	writer.writeValues(transactions.size());

	for (const Transaction& transaction : transactions) {
		const std::string serializedTransaction = transaction.serializeTransaction();
		writer.writeValues(serializedTransaction.size());
		writer.writeBytes(serializedTransaction);
	}
	return writer.getStringBytes();
}
