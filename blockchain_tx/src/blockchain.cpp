#include <blockchain.h>

using namespace drogon;

// TODO: Make the path portable
Blockchain::Blockchain() : utxoDB("utxo"), transactionDB("transactions"), blocksDB("blocks"), height{0} {
	loadTransactions();
	loadBlocks();
	if (blocks.empty()) createGenesisBlock();
	LOG_INFO << "Transactions loaded";
	LOG_INFO << "Blocks loaded";
}

void Blockchain::createGenesisBlock() {
	Hash hash{};
	Hash previousHash{};

	Addr shaik{};
	Hash mHash{};

	sodium_hex2bin(
		hash.data(), hash.size(),
		"006ec02d681c34244f828732237213d5e4b485f4abe1fe875206dcff40375ee9",
		64,
		nullptr, nullptr, nullptr
	);

	sodium_hex2bin(
		shaik.data(), shaik.size(),
		"b4720462ac198c6e6a55a89de9445498a64406aa",
		40,
		nullptr, nullptr, nullptr
	);

	sodium_hex2bin(
		mHash.data(), mHash.size(),
		"e5d79d482d0e7ab7cbf67d56e59eaebc04893a3cf57766d97c311f2a9c28db2e",
		64,
		nullptr, nullptr, nullptr
	);


	UTXO utxo{ shaik, 500 };
	Transaction tx{ shaik, shaik, 500, {}, {utxo} };
	tx.timestamp = 1780775044370156300;

	Block gBlock{ previousHash, hash, 1780775044373228000 , 29, {tx} };
	gBlock.blockHeader.merkleRoot = mHash;
	blocks.push_back(gBlock);

	uint64_t height = blocks.size();
	std::stringstream ss;
	ss << std::setw(10) << std::setfill('0') << height;
	std::string key = ss.str();

	blocksDB.saveKey(key, serializeBlock(gBlock));
}

Blockchain& Blockchain::getInstance() {
	static Blockchain blockchainInstance;
	return blockchainInstance;
}

void Blockchain::loadBlocks() {
	std::unique_ptr<leveldb::Iterator> it(
		blocksDB.db->NewIterator(leveldb::ReadOptions())
	);
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string value = it->value().ToString();
		Block block = deserializeBlock(value);
		blocks.push_back(block);
	}

	if (!it->status().ok()) {
		std::cerr << "Iterator error: " << it->status().ToString() << std::endl;
	}
}

void Blockchain::loadTransactions() {
	std::unique_ptr<leveldb::Iterator> it(
		transactionDB.db->NewIterator(leveldb::ReadOptions())
	);
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string value = it->value().ToString();
		Transaction transaction{ value };
		if (transactions.find(Common::toHex(transaction.transaction_hash)) != transactions.end()) {
			LOG_ERROR << "DUPLICATE TRANSACTION";
			continue;
		}
		transactions.emplace(Common::toHex(transaction.transaction_hash), transaction);
	}

	if (!it->status().ok()) {
		std::cerr << "Iterator error: " << it->status().ToString() << std::endl;
	}

	for (const auto& [txid, tx] : transactions) {
		updateUTXO(tx);
	}
}

void Blockchain::spareCoins(const Addr& owner) {
	Input input{ Common::hashBytes(owner), 1 };
	utxo.emplace(input.getUTXOKey(), UTXO{ owner, 1000 });
}

UTXOResult Blockchain::getUTXO(const Addr& addr, uint64_t coins) {
	UTXOResult result;
	for (const auto& [txid, out] : utxo) {
		if (out.owner == addr) {
			result.total += out.coins;
			std::string tx_hash_hex{txid.data(), TransactionHashSize * 2};
			TransactionHash tx_hash = Common::toBytes<TransactionHashSize>(tx_hash_hex);
			uint32_t output_index = static_cast<uint32_t>(std::stoul(txid.data() + (TransactionHashSize * 2) + 1));
			result.inputs.emplace_back(tx_hash, output_index);
			if (result.total >= coins) break;
		}
	}
	return result;
}

bool Blockchain::verifySignature(const SignedTransaction& st) {
	if (crypto_sign_verify_detached(st.signature.data(), st.transaction.transaction_hash.data(), st.transaction.transaction_hash.size(), st.publicKey.data()) != 0) {
		LOG_ERROR << "SIGNATURE VERIFICATION FAILED";
		return false;
	}
	Addr address = Common::computeAddress(st.publicKey);
	for (const auto& in : st.transaction.inputs) {
		const auto it = utxo.find(in.getUTXOKey());
		if (it == utxo.end()) return false;
		if (it->second.owner != address) {
			LOG_ERROR << "NOT YOUR INPUTS";
			return false;
		}
	}
	return true;
}

bool Blockchain::verifyTransaction(const Transaction& transaction) {
	uint64_t coins = 0;
	for (const auto& in : transaction.inputs) {
		auto it = utxo.find(in.getUTXOKey());
		if (it == utxo.end()) {
			LOG_ERROR << "INVALID INPUTS";
			return false;
		}
		coins += it->second.coins;
	}
	if (coins < transaction.coins) {
		LOG_ERROR << "INSUFFICIENT BALANCE";
		return false;
	}
	return true;
}

void Blockchain::updateUTXO(const Transaction& transaction) {
	for (const auto& in : transaction.inputs) 
		utxo.erase(in.getUTXOKey());
	for (size_t i = 0; i < transaction.outputs.size(); i++) {
		std::string utxoKey;
		utxoKey.reserve(TransactionHashSize * 2 + 1 + 10);
		utxoKey.append(Common::toHex(transaction.transaction_hash));
		utxoKey.push_back(':');
		utxoKey.append(std::to_string(i));
		utxo.emplace(utxoKey, transaction.outputs[i]);
	}
}

void Blockchain::addTransaction(const SignedTransaction& signedTransaction) {
	if (!verifyTransaction(signedTransaction.transaction)) return;
	if (!verifySignature(signedTransaction)) return;
	transactions.emplace(Common::toHex(signedTransaction.transaction.transaction_hash), signedTransaction.transaction);
	updateUTXO(signedTransaction.transaction);
	transactionsPool.push_back(signedTransaction.transaction);
	LOG_INFO << "TRANSACTION ADDED TO POOL";
}

Hash Blockchain::getCurrentBlockHash() const {
	return blocks.back().blockHeader.hash;
}

const std::vector<Transaction>& Blockchain::getTXPool() const {
	return transactionsPool;
}

void Blockchain::listTransactions() const {
	tabulate::Table transactions_table;
	transactions_table.add_row({ "Transaction Hash", "Sender", "Receiver", "Coins", "Timestamp"});
	for (const auto& [txid, tx] : transactions) {
		transactions_table.add_row({Common::toHex(tx.transaction_hash), Common::toHex(tx.sender), Common::toHex(tx.receiver), std::to_string(tx.coins), std::to_string(tx.timestamp)});
	}
	std::cout << transactions_table << "\nBlocks size: " << blocks.size() << "\n";
}

void Blockchain::listBlocks() {
	for (size_t i = 0; i < blocks.size(); i++) {
		std::cout << "Block " << i << "\n\t\tHash: " << Common::toHex(blocks[i].blockHeader.hash) << "\n\t\tPrevious hash: " << Common::toHex(blocks[i].blockHeader.previous_hash) << "\n\t\tNonce: " << blocks[i].blockHeader.nonce << "\n";
	}
}

void Blockchain::listUTXO() const {
	tabulate::Table utxo_table;
	utxo_table.add_row({ "UTXO Key", "Owner", "Coins" });
	for (const auto& [utxo_key, out] : utxo) {
		utxo_table.add_row({ utxo_key, Common::toHex(out.owner), std::to_string(out.coins)});
	}
	std::cout << utxo_table << "\n";
}

std::string Blockchain::serializeBlock(const Block& block) {
	std::string base;
	size_t transactionsSize = block.transactions.size();

	base.reserve(
		block.blockHeader.previous_hash.size() +
		block.blockHeader.merkleRoot.size() +
		block.blockHeader.hash.size() + sizeof(transactionsSize) +
		sizeof(uint64_t) +
		sizeof(uint64_t)
	);

	base.append(reinterpret_cast<const char*>(block.blockHeader.previous_hash.data()), block.blockHeader.previous_hash.size());
	base.append(reinterpret_cast<const char*>(block.blockHeader.hash.data()), block.blockHeader.hash.size());
	base.append(reinterpret_cast<const char*>(block.blockHeader.merkleRoot.data()), block.blockHeader.merkleRoot.size());
	base.append(reinterpret_cast<const char*>(&block.blockHeader.nonce), sizeof(block.blockHeader.nonce));
	base.append(reinterpret_cast<const char*>(&block.blockHeader.timestamp), sizeof(block.blockHeader.timestamp));
	base.append(reinterpret_cast<const char*>(&transactionsSize), sizeof(transactionsSize));
	for (const Transaction& t : block.transactions) {
		std::string serializedTX = t.serializeTransaction();
		size_t txSize = serializedTX.size();
		base.append(reinterpret_cast<const char*>(&txSize), sizeof(txSize));
		base.append(reinterpret_cast<const char*>(serializedTX.data()), serializedTX.size());
	}
	return base;
}

Block Blockchain::deserializeBlock(const std::string& bytes) {
	// [previousHash][hash][merkleRoot][nonce][timestamp]
	Hash previousHash{};
	Hash hash{};
	Hash merkleRoot{};
	uint64_t nonce{};
	uint64_t timestamp{};
	size_t transactionsSize{};
	std::vector<Transaction> txs;

	std::memcpy(previousHash.data(), bytes.data(), previousHash.size());
	std::memcpy(hash.data(), bytes.data() + previousHash.size(), hash.size());
	std::memcpy(merkleRoot.data(), bytes.data() + previousHash.size() + hash.size(), merkleRoot.size());
	std::memcpy(&nonce, bytes.data() + previousHash.size() + hash.size() + merkleRoot.size(), sizeof(nonce));
	std::memcpy(&timestamp, bytes.data() + previousHash.size() + hash.size() + merkleRoot.size() + sizeof(nonce), sizeof(timestamp));
	std::memcpy(&transactionsSize, bytes.data() + previousHash.size() + hash.size() + merkleRoot.size() + sizeof(nonce) + sizeof(timestamp), sizeof(transactionsSize));

	size_t offset = previousHash.size() + hash.size() + merkleRoot.size()
		+ sizeof(nonce) + sizeof(timestamp) + sizeof(transactionsSize);

	for (size_t i = 0; i < transactionsSize; i++) {
		size_t txSize{};
		std::memcpy(&txSize, bytes.data() + offset, sizeof(txSize));
		offset += sizeof(txSize);

		std::string txBytes;
		txBytes.resize(txSize);
		std::memcpy(txBytes.data(), bytes.data() + offset, txSize);
		offset += txSize;

		txs.emplace_back(txBytes);
	}

	Block block{ previousHash, hash, timestamp, nonce, txs };
	block.blockHeader.merkleRoot = merkleRoot;
	return block;

}
