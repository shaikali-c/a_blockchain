#pragma once
#include "pch.h"
#include "axis/blockchain/transaction.h"
#include "axis/core/common.h"
#include "axis/crypto/cryptography.h"

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
	explicit Block(std::string_view rawBytes);
	[[nodiscard]] std::string serialize() const;
};
