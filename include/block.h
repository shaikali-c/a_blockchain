#pragma once
#include "pch.h"
#include "transaction.h"
#include "common.h"
#include "cryptography.h"

struct BlockHeader {
	Hash previous_hash;
	Hash hash;
	uint64_t timestamp;
	uint64_t nonce;
	Hash merkleRoot;
};

class Block {
public:
	BlockHeader blockHeader;
	std::vector<Transaction> transactions;
	Block(
		const Hash& ph,
		const Hash& bh,
		uint64_t t,
		uint64_t n,
		const std::vector<Transaction>& txs
	);
	Block(const std::string rawBytes);
};