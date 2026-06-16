#include <blockchain.h>

// TODO: Make the path portable
Blockchain::Blockchain() : transactionDB("transactions"), blocksDB("blocks"), poolsDB("pool"), height{0}, difficulty(1) {
	loadBlocks();
	loadPoolTransactions();
	if (blocks.empty()) createGenesisBlock();
	buildTarget();
}

void Blockchain::createGenesisBlock() {
	Hash hash = Common::toBytes < Hash{}.size() > ("00ed2b18a04fcda54d010e119158382d75b70de330a7f532dfb1734a6cef52de");
	Hash previousHash{};
	previousHash.fill(0x00);

	Addr shaik = Common::toBytes < Addr{}.size() >("f45a20e043b01f65638a46831ce79b8fec3f6737");
	Hash mHash = Common::toBytes < Hash{}.size() > ("45f81bc87cb00781d3e9ca528b6720fe266fda2e9bed256994ad4046e21d0060");;

	UTXO utxo{ shaik, 500 };
	Transaction tx{ shaik, shaik, 500, {}, {utxo}, 1781545365 };
	uint64_t nonce = 74;

	Block gBlock{ previousHash, hash, 1781545365 , nonce, {tx} };
	gBlock.blockHeader.merkleRoot = mHash;
	blocks.push_back(gBlock);
	blocksDB.saveKey(generateBlockKey(), serializeBlock(gBlock));
}

Blockchain& Blockchain::getInstance() {
	static Blockchain blockchainInstance;
	return blockchainInstance;
}
std::string Blockchain::generateBlockKey() {
	uint64_t height = blocks.size();
	std::stringstream ss;
	ss << std::setw(10) << std::setfill('0') << height;
	std::string key = ss.str();
	return key;
}

Hash Blockchain::buildTarget() {
	Hash t{};
	t.fill(0xff);
	for (int i = 0; i < difficulty; i++) t[i] = 0x00;
	target = t;
	return t;
}

bool Blockchain::verifyDifficulty(const Hash& hash) {
	if (target == Hash{}) buildTarget();
	return hash <= target;
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
	loadTransactions();
}

void Blockchain::loadTransactions() {
	for (const auto& b : blocks) {
		for (const auto& t : b.transactions) {
			transactions.emplace(Common::toHex(t.transaction_hash), t);
		}
	}
	for (const auto& [txid, tx] : transactions) {
		updateUTXO(tx);
	}
}

void Blockchain::loadPoolTransactions() {
	std::unique_ptr<leveldb::Iterator> it(
		poolsDB.db->NewIterator(leveldb::ReadOptions())
	);
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string key = it->key().ToString();
		std::string value = it->value().ToString();
		Transaction tx{value};
		transactionsPool.emplace(key, tx);
	}
}

bool Blockchain::verifySignature(const SignedTransaction& st) {
	return crypto_sign_verify_detached(st.signature.data(), st.transaction.transaction_hash.data(), st.transaction.transaction_hash.size(), st.publicKey.data()) == 0;
}

bool Blockchain::verifyTransaction(const Transaction& transaction) {
	uint64_t coins = 0;
	for (const auto& in : transaction.inputs) {
		auto it = utxo.find(in.getUTXOKey());
		if (it == utxo.end()) {
			return false;
		}
		coins += it->second.coins;
	}
	if (coins < transaction.coins) {
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

std::expected<void, std::string> Blockchain::addTransaction(const SignedTransaction& signedTransaction) {
	if (!verifyTransaction(signedTransaction.transaction)) return std::unexpected("Transaction verification failed");
	if (!verifySignature(signedTransaction)) return std::unexpected("Signature verification failed");
	transactions.emplace(Common::toHex(signedTransaction.transaction.transaction_hash), signedTransaction.transaction);
	updateUTXO(signedTransaction.transaction);

	auto transactionHashHex = Common::toHex < Hash{}.size() > (signedTransaction.transaction.transaction_hash);
	transactionsPool.emplace(transactionHashHex, signedTransaction.transaction);
	poolsDB.saveKey(transactionHashHex, signedTransaction.transaction.serializeTransaction());
	return {};
}

Hash Blockchain::getCurrentBlockHash() const {
	return blocks.back().blockHeader.hash;
}

bool Blockchain::transactionInPool(const std::string& txHash) const {
	return transactionsPool.find(txHash) != transactionsPool.end();
}


void Blockchain::listTransactions() const {
	tabulate::Table transactions_table;
	transactions_table.add_row({ "Transaction Hash", "Sender", "Receiver", "Coins", "Timestamp"});
	for (const auto& [txid, tx] : transactions) {
		transactions_table.add_row({Common::toHex(tx.transaction_hash), Common::toHex(tx.sender), Common::toHex(tx.receiver), std::to_string(tx.coins), std::to_string(tx.timestamp)});
	}
	std::cout << transactions_table << "\n";
}

void Blockchain::listPoolTransactions() const {
	tabulate::Table transactions_table;
	transactions_table.add_row({ "Transaction Hash", "Sender", "Receiver", "Coins", "Timestamp" });
	for (const auto& [txid, tx] : transactionsPool) {
		transactions_table.add_row({ Common::toHex(tx.transaction_hash), Common::toHex(tx.sender), Common::toHex(tx.receiver), std::to_string(tx.coins), std::to_string(tx.timestamp)});
	}
	std::cout << transactions_table << "\n";
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
	BytesWriter buffer;
	buffer.writeBytes(block.blockHeader.previous_hash);
	buffer.writeBytes(block.blockHeader.hash);
	buffer.writeBytes(block.blockHeader.merkleRoot);
	buffer.writeValues(block.blockHeader.nonce);
	buffer.writeValues(block.blockHeader.timestamp);
	buffer.writeValues(block.transactions.size());
	for (const Transaction& t : block.transactions) {
		std::string serializedTX = t.serializeTransaction();
		size_t transactionDataSize = serializedTX.size();
		buffer.writeValues(transactionDataSize);
		buffer.writeBytes(serializedTX);
	}
	return buffer.getStringBytes();
}

Block Blockchain::deserializeBlock(const std::string& bytes) {
	// [previousHash][hash][merkleRoot][nonce][timestamp]
	BytesReader reader{ bytes };
	Hash previousHash = reader.readBytes<Hash>();
	Hash hash = reader.readBytes<Hash>();
	Hash merkleRoot = reader.readBytes<Hash>();
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
	return block;

}

crow::response Blockchain::getUTXO(const std::string& address) {
	nlohmann::json json;
	if (address.size() != Addr{}.size() * 2) {
		json["error"] = "Invalid address";
		return crow::response{ 400, json.dump() };
	}
	Addr addr = Common::toBytes < Addr{}.size() > (address);
	uint64_t coins{};
	for (const auto& [utxoKey, out] : utxo) {
		if (out.owner == addr) {
            nlohmann::json utxoJson;
            Input input{utxoKey};
            utxoJson["utxoKey"] = Common::toHex(input.transaction_hash);
            utxoJson["outputIndex"] = input.output_index;
			json["utxos"].push_back(utxoJson);
			coins += out.coins;
		}
	}
	json["coins"] = coins;

	crow::response res;
	res.code = 200;
	res.set_header("Content-Type", "application/json");
	res.body = json.dump();
	return res;
}

crow::response Blockchain::createTransaction(const crow::request& req) {
	auto json = nlohmann::json::parse(req.body);
	nlohmann::json responseJson;
	nlohmann::json errorJson;
	crow::response res;

	auto senderPK_ = getBytes < PublicKey{}.size() > (json, "sender", errorJson, res);
	auto receiver_ = getBytes < Addr{}.size() > (json, "receiver", errorJson, res);
	auto signature_ = getBytes < Signature{}.size() > (json, "signature", errorJson, res);
	auto amount_ = getField<uint64_t>(json, "amount", errorJson, res);
	auto timestamp_ = getField<uint64_t>(json, "timestamp", errorJson, res);

	if (!senderPK_|| !receiver_|| !signature_ || !amount_ || !timestamp_) return res;

	const auto& senderPK = *senderPK_;
	const auto& receiver = *receiver_;
	const auto& signature = *signature_;
	const auto& amount = *amount_;
	const auto& timestamp = *timestamp_;

	Addr sender = Common::computeAddress(senderPK);

	std::vector<Input> inputs;
	std::vector<UTXO> outputs;

	for (const auto& i : json["inputs"]) inputs.emplace_back(i);
	for (const auto& o : json["outputs"]) {
		Addr addr = Common::toBytes < Addr{}.size() > (o["address"].get<std::string>());
		uint64_t coins = o["coins"].get<uint64_t>();
		outputs.emplace_back(addr, coins);
	}
	Transaction tx{ sender, receiver, amount, inputs, outputs, timestamp };
	SignedTransaction sTransaction{ tx, senderPK, signature };

	res.code = 200;
	res.set_header("Content-Type", "application/json");
	auto vTransaction = addTransaction(sTransaction);
	if (!vTransaction) {
		responseJson["error"] = vTransaction.error();
		return responseJson.dump();
	}
	nlohmann::json successJson;
	successJson["success"] = "Transaction added to pool";
	return successJson.dump();
}

// Note: Sender will always send public keys, always

crow::response Blockchain::createBlock(const crow::request& req) {
	auto json = nlohmann::json::parse(req.body);
	nlohmann::json errorJson;
	nlohmann::json successJson;
	crow::response res;

	auto hash = getBytes < Hash{}.size() > (json, "hash", errorJson, res);
	auto merkleRoot = getBytes < Hash{}.size() > (json, "merkleRoot", errorJson, res);
	auto previousHash = getBytes < Hash{}.size() > (json, "previousHash", errorJson, res);
	auto minerAddress = getBytes < Addr{}.size() > (json, "minerAddress", errorJson, res);
	auto nonce = getField<uint64_t>(json, "nonce", errorJson, res);
	auto timestamp = getField<uint64_t>(json, "timestamp", errorJson, res);

	if (!hash || !merkleRoot || !previousHash || !minerAddress || !nonce || !timestamp) return res;

	std::vector<Hash> transactionHashes;
	transactionHashes.emplace_back(Common::toBytes < Hash{}.size() > (json["transactions"][0]));

	res.set_header("Content-Type", "application/json");
	for (size_t i = 1; i < json["transactions"].size(); i++) {
		if (transactionsPool.find(json["transactions"][i]) == transactionsPool.end()) {
			res.code = 500;
			errorJson["error"] = "Invalid transaction provided";
			res.body = errorJson.dump();
			return res;
		}
		transactionHashes.emplace_back(Common::toBytes < Hash{}.size() > (json["transactions"][i]));
	}

	Hash expectedMerkleRoot = Cryptography::computeMerkleRoot(transactionHashes);
	if (transactionHashes.size() > 1 && expectedMerkleRoot != *merkleRoot) {
		errorJson["error"] = "Merkle root doesn't match";
		res.code = 500;
		res.body = errorJson.dump();
		return res;
	}

	if (*previousHash != blocks.back().blockHeader.hash) {
		errorJson["error"] = "Previous hash doesn't match";
		res.code = 500;
		res.body = errorJson.dump();
		return res;
	}

	if (!verifyDifficulty(*hash)) {
		errorJson["error"] = "Hash should be less or equal to target";
		res.code = 500;
		res.body = errorJson.dump();
		return res;
	}

	std::vector<Transaction> blockTransactions;
	UTXO minerUTXO{ *minerAddress, MINER_REWARD };
	Transaction tx{ *minerAddress, MINER_REWARD, {minerUTXO} };
	blockTransactions.push_back(tx);
	for (size_t i = 1; i < transactionHashes.size(); i++) {
		const auto& t = transactionHashes[i];
		std::string txHashHex = Common::toHex(t);

		auto it = transactionsPool.find(txHashHex);
		if (it == transactionsPool.end()) {
			errorJson["error"] = "Transaction not found in pool: " + txHashHex;
			res.code = 500;
			res.body = errorJson.dump();
			return res;
		}
		blockTransactions.push_back(it->second);
		poolsDB.remove(it->first);
		transactionsPool.erase(it);
	}
	blocks.emplace_back(*previousHash, *hash, *timestamp, *nonce, blockTransactions);
	blocksDB.saveKey(generateBlockKey(), serializeBlock(blocks.back()));

	successJson["success"] = "Block been added";
	res.code = 200;
	res.body = successJson.dump();
	return res;
}

void Blockchain::startServer() {
	crow::SimpleApp server;
	server.loglevel(crow::LogLevel::Info);
	CROW_ROUTE(server, "/utxo/<string>")([this](std::string address) {
		return getUTXO(address);
	});
	CROW_ROUTE(server, "/createTransaction").methods(crow::HTTPMethod::Post)([this](const crow::request& req) {
		return createTransaction(req);
	});

	CROW_ROUTE(server, "/validateBlock").methods(crow::HTTPMethod::Post)([this](const crow::request& req) {
		return createBlock(req);
	});
	server.port(18080).run();
}
