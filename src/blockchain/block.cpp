#include "axis/blockchain/block.h"

namespace {

std::vector<Transaction> readTransactions(BytesReader& reader, size_t transactionsCount) {
	if (transactionsCount > reader.remaining() / sizeof(size_t)) {
		throw std::runtime_error("Invalid block transaction count");
	}

	std::vector<Transaction> transactions;
	transactions.reserve(transactionsCount);
	for (size_t i = 0; i < transactionsCount; ++i) {
		const size_t transactionDataSize = reader.readBytes<size_t>();
		const std::string transactionBytes = reader.readBytesString(transactionDataSize);
		transactions.emplace_back(transactionBytes);
	}
	return transactions;
}

} // namespace

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
	BytesReader reader{ rawBytes };

	const Hash previousHash = reader.readBytes<HashSize>();
	const Hash hash = reader.readBytes<HashSize>();
	const Hash merkleRoot = reader.readBytes<HashSize>();
	const uint64_t nonce = reader.readBytes<uint64_t>();
	const uint64_t timestamp = reader.readBytes<uint64_t>();
	const size_t transactionsCount = reader.readBytes<size_t>();

	blockHeader = { previousHash, hash, timestamp, nonce, merkleRoot };
	transactions = readTransactions(reader, transactionsCount);
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
