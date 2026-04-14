#include "pch.h"
#include "transaction.h"
#include "common.h"

struct BlockHeader {
	Hash previous_hash;
	Hash hash;
	Hash merkleRoot;
	uint64_t nonce;
	uint64_t timestamp;
};

class Block {
public:
	BlockHeader blockHeader;
	std::vector<Transaction> transactions;
	Block(
		const Hash& bh,
		const Hash& ph,
		uint64_t n,
		uint64_t t,
		const std::vector<Transaction>& txs
	) : blockHeader{ph, bh, bh, n, t}, transactions(txs) {
}
};