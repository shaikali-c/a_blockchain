#pragma once
#include "pch.h"
#include "common.h"
#include "block.h"

struct Miner {
	uint64_t nonce;
	uint64_t timestamp;
	Hash hash;
	Hash merkleRoot;
	Block* block;
	Hash pHash;
	uint32_t difficulty;
	std::vector<Transaction> transactions;

	Miner(Hash pHash, std::vector<Transaction> txs) : transactions(txs), nonce(0), difficulty(1), pHash(pHash) {
		timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count();
		std::vector<Hash> txHashes;
		for (const auto& t : transactions)
			txHashes.push_back(t.transaction_hash);
		merkleRoot = Cryptography::computeMerkleRoot(txHashes);
	}
	bool checkHash() {
		return std::all_of(
			hash.begin(),
			hash.begin() + difficulty,
			[](unsigned char b) { return b == 0; }
		);
	}
	Block mine() {
		block = new Block{ pHash, hash, timestamp, nonce, transactions };
		(*block).blockHeader.merkleRoot = merkleRoot;


		constexpr size_t bytesSize = Hash{}.size() * 2 + sizeof(nonce) + sizeof(timestamp);
		std::array<unsigned char, bytesSize> bytes{};
		std::memcpy(bytes.data(), pHash.data(), pHash.size());
		std::memcpy(bytes.data() + pHash.size(), merkleRoot.data(), merkleRoot.size());
		std::memcpy(bytes.data() + Hash{}.size() * 2, &timestamp, sizeof(timestamp));
		constexpr size_t nonceOffset = Hash{}.size() * 2 + sizeof(timestamp);
		while (true) {
			std::memcpy(bytes.data() + nonceOffset, &nonce, sizeof(nonce));
			hash = Common::hashBytes(bytes);
			if (checkHash()) {
				(*block).blockHeader.hash = hash;
				(*block).blockHeader.nonce = nonce;
				(*block).blockHeader.timestamp = timestamp;
				return *block;
			}
			++nonce;
		}
	}
};
