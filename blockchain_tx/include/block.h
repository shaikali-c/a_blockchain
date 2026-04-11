#include "pch.h"
#include "transaction.h"
#include "common.h"

class Block {
public:
	Hash block_hash;
	Hash previous_hash;
	uint64_t nonce;
	uint64_t timestamp;
	std::vector<Transaction> transactions;
	Block(
		const Hash& bh,
		const Hash& ph,
		uint64_t n,
		uint64_t t,
		const std::vector<Transaction>& txs
	) : block_hash(bh), previous_hash(ph), nonce(n), timestamp(t), transactions(txs){}
};