#include "block.h"

Block::Block(
	const Hash& ph,
	const Hash& bh,
	uint64_t t,
	uint64_t n,
	const std::vector<Transaction>& txs
) : blockHeader{ ph, bh, t, n }, transactions(txs) {
	std::vector<TransactionHash> tx_hashes;
	for (const auto t : txs)
		tx_hashes.push_back(t.transaction_hash);
	blockHeader.merkleRoot = Cryptography::computeMerkleRoot(tx_hashes);
}