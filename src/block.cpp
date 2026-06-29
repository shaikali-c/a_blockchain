#include "block.h"

Block::Block(
	const Hash& ph,
	const Hash& bh,
	uint64_t t,
	uint64_t n,
	const std::vector<Transaction>& txs
) : blockHeader{ ph, bh, t, n }, transactions(txs) {
	std::vector<Hash> tx_hashes;
	for (const auto t : txs)
		tx_hashes.push_back(t.transaction_hash);
	blockHeader.merkleRoot = Cryptography::computeMerkleRoot(tx_hashes);
}

Block::Block(const std::string rawBytes) {
	// [previousHash][hash][merkleRoot][nonce][timestamp]
	BytesReader reader{ rawBytes };
	Hash previousHash = reader.readBytes < HashSize > ();
	Hash hash = reader.readBytes<HashSize>();
	Hash merkleRoot = reader.readBytes<HashSize>();
	uint64_t nonce = reader.readBytes<uint64_t>();
	uint64_t timestamp = reader.readBytes<uint64_t>();
	size_t transactionsCount = reader.readBytes<size_t>();
	std::vector<Transaction> txs;

	for (size_t i = 0; i < transactionsCount; i++) {
		size_t transactionDataSize = reader.readBytes<size_t>();
		std::string transactionBytes = reader.readBytesString(transactionDataSize);
		txs.emplace_back(transactionBytes);
	}

	Block block{ previousHash, hash, timestamp, nonce, txs };
	block.blockHeader.merkleRoot = merkleRoot;
}