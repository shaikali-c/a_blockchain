#include <blockchain.h>

// TODO: Make the path portable
// TODO: Make UTXO find much faster by maintaining another set with sorted coins
// TODO: Make difficulty adjusted based on blocks been mined every minute

Blockchain::Blockchain() : blocksDB("blocks"), poolsDB("pool"), height{ 0 }, difficulty(3) {
	loadBlocks();
	loadPoolTransactions();
	if (blocks.empty()) createGenesisBlock();
	buildTarget();
}

void Blockchain::createGenesisBlock() {
	Hash previousHash{};
	previousHash.fill(0x00);
	Hash hash = toBytes < Hash{}.size() > ("00007f009a96c42684187a363b47504d5461b99852853670d4edd3adc4ea777e");
	Hash mHash = toBytes < Hash{}.size() > ("22e89138181eeff1e1b80dd0aa5467b47fa65fb3036bb835f6de0d3917ba8efc");;

	Addr shaik = toBytes < Addr{}.size() > ("f45a20e043b01f65638a46831ce79b8fec3f6737");

	UTXO utxo{ shaik, GENESIS_REWARD };
	Transaction tx{ shaik, utxo.owner, utxo.coins, {}, {utxo}, 1781545365 };
	transactions.emplace(toHex(tx.transaction_hash), tx);
	uint64_t nonce = 31496, timestamp = 1781545365; // These values were mined and hardcoded into the genesis block to ensure all nodes share the same chain origin

	Block gBlock{ previousHash, hash, timestamp , nonce, std::vector{tx} };
	gBlock.blockHeader.merkleRoot = mHash;
	blocks.push_back(gBlock);
	blocksDB.saveKey(generateBlockKey(), serializeBlock(gBlock));
	updateUTXO(tx);
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

bool Blockchain::verifyInputs(const SignedTransaction& st) {
	Addr address = computeAddress(st.publicKey);
	uint64_t total_inputs = 0;
	uint64_t total_output = 0;
	for (const auto& in : st.transaction.inputs) {
		auto it = utxo.find(in.getUTXOKey());
		if (it == utxo.end()) return false;
		if (it->second.owner != address) return false;
		total_inputs += it->second.coins;
	}
	for (const auto& out : st.transaction.outputs) total_output += out.coins;
	return total_inputs >= total_output;
}

void Blockchain::loadBlocks() {
	std::unique_ptr<leveldb::Iterator> it(
		blocksDB.db->NewIterator(leveldb::ReadOptions())
	);
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string value = it->value().ToString();
		Block block = deserializeBlock(value);
		for (const auto& t : block.transactions) {
			transactions.emplace(toHex(t.transaction_hash), t);
			updateUTXO(t);
		}
		blocks.push_back(block);
		auto hashHex = toHex(block.blockHeader.hash);
		blocksMap[hashHex] = blocks.size() - 1;
	}

	if (!it->status().ok()) {
		std::cerr << "Iterator error: " << it->status().ToString() << std::endl;
	}
	Logger::log("BLOCKS LOADED");
}

void Blockchain::loadPoolTransactions() {
	std::unique_ptr<leveldb::Iterator> it(
		poolsDB.db->NewIterator(leveldb::ReadOptions())
	);
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string key = it->key().ToString();
		std::string value = it->value().ToString();
		Transaction tx{ value };
		transactionsPool.emplace(key, tx);
		for (const auto& i : tx.inputs)
			mempoolInputs[i.getUTXOKey()] = i;
	}
	Logger::log("POOL LOADED");
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
		if (it->second.owner != transaction.sender)
			return false;
		coins += it->second.coins;
	}
	if (coins < transaction.coins) return false;
	return true;
}

void Blockchain::updateUTXO(const Transaction& transaction) {
	for (const auto& in : transaction.inputs)
		utxo.erase(in.getUTXOKey());
	for (size_t i = 0; i < transaction.outputs.size(); i++) {
		std::string utxoKey;
		utxoKey.reserve(TransactionHashSize * 2 + 1 + 10);
		utxoKey.append(toHex(transaction.transaction_hash));
		utxoKey.push_back(':');
		utxoKey.append(std::to_string(i));
		utxo.emplace(utxoKey, transaction.outputs[i]);
	}
}

std::expected<void, std::string> Blockchain::addTransaction(const SignedTransaction& signedTransaction) {
	if (signedTransaction.transaction.coins <= 0) return std::unexpected("Invalid transaction amount");
	if (!verifyTransaction(signedTransaction.transaction)) return std::unexpected("Transaction verification failed");
	if (!verifyInputs(signedTransaction)) return std::unexpected("Ownership verification failed");
	if (!verifySignature(signedTransaction)) return std::unexpected("Signature verification failed");
	auto transactionHashHex = toHex(signedTransaction.transaction.transaction_hash);
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
	if (!transactions.empty()) {
		tabulate::Table transactions_table;
		transactions_table.add_row({ "Transaction Hash", "Sender", "Receiver", "Coins", "Timestamp" });
		for (const auto& [txid, tx] : transactions) {
			transactions_table.add_row({ toHex(tx.transaction_hash), toHex(tx.sender), toHex(tx.receiver), std::to_string(tx.coins), std::to_string(tx.timestamp) });
		}
		std::cout << transactions_table << "\n";
	}
}

void Blockchain::listPoolTransactions() const {
	if (!transactionsPool.empty()) {
		tabulate::Table transactions_table;
		transactions_table.add_row({ "Pool Hash", "Sender", "Receiver", "Coins", "Timestamp" });
		for (const auto& [txid, tx] : transactionsPool) {
			transactions_table.add_row({ toHex(tx.transaction_hash), toHex(tx.sender), toHex(tx.receiver), std::to_string(tx.coins), std::to_string(tx.timestamp) });
		}
		std::cout << transactions_table << "\n";
	}

}

void Blockchain::listBlocks() {
	for (size_t i = 0; i < blocks.size(); i++) {
		std::cout << "Block " << i << "\n\t\tHash: " << toHex(blocks[i].blockHeader.hash) << "\n\t\tPrevious hash: " << toHex(blocks[i].blockHeader.previous_hash) << "\n\t\tNonce: " << blocks[i].blockHeader.nonce << "\n";
	}
}

crow::response Blockchain::getBalance(const Addr& address) {
	crow::response response;
	nlohmann::json responseJson, blockJson;
	response.set_header("Content-Type", "application/json");
	response.add_header("Access-Control-Allow-Origin", "*");
	response.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	response.add_header("Access-Control-Allow-Headers", "Content-Type");

	uint64_t c = 0;
	for (const auto& [utxoKey, out] : utxo) {
		if (out.owner == address)
			c += out.coins;
	}
	responseJson["coins"] = c;
	response.code = 200;
	response.body = responseJson.dump();
	return response;
}


void Blockchain::listUTXO() const {
	tabulate::Table utxo_table;
	utxo_table.add_row({ "UTXO Key", "Owner", "Coins" });
	for (const auto& [utxo_key, out] : utxo) {
		utxo_table.add_row({ utxo_key, toHex(out.owner), std::to_string(out.coins / UNITS) });
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

crow::response Blockchain::getUTXO(const crow::request& req) {
	auto json = nlohmann::json::parse(req.body);
	nlohmann::json responseJson, errorJson;

	crow::response response;
	response.set_header("Content-Type", "application/json");
	response.add_header("Access-Control-Allow-Origin", "*");
	response.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	response.add_header("Access-Control-Allow-Headers", "Content-Type");

	auto address_ = getBytes < Addr{}.size() > (json, "address", errorJson, response);
	auto coins_ = getField<uint64_t>(json, "coins", errorJson, response);

	if (!address_ || !coins_) return response;

	const auto& address = *address_;
	const uint64_t& coins = *coins_;

	uint64_t c = 0;
	for (const auto& [utxoKey, out] : utxo) {
		if (out.owner == address) {
			nlohmann::json utxoJson;
			Input input{ utxoKey };
			utxoJson["utxoKey"] = toHex(input.transaction_hash);
			utxoJson["outputIndex"] = input.output_index;
			json["utxos"].push_back(utxoJson);
			c += out.coins;
			if (c >= coins) break;
		}
	}
	json["coins"] = c;
	if (!c) {
		json["utxos"] = nlohmann::json::array();
	}
	crow::response res;
	res.set_header("Content-Type", "application/json");
	res.code = 200;
	res.body = json.dump();
	return res;
}

crow::response Blockchain::getTransaction(const std::string& hash) {
	nlohmann::json responseJson, transactionJson;
	crow::response response;
	response.set_header("Content-Type", "application/json");
	response.add_header("Access-Control-Allow-Origin", "*");
	response.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	response.add_header("Access-Control-Allow-Headers", "Content-Type");

	auto it = transactions.find(hash);
	if (it == transactions.end()) {
		responseJson["error"] = "Transaction not found";
		response.code = 404;
		response.body = responseJson.dump();
		return response;
	}
	transactionJson["transactionHash"] = hash;
	transactionJson["sender"] = toHex(it->second.sender);
	transactionJson["receiver"] = toHex(it->second.receiver);
	transactionJson["timestamp"] = it->second.timestamp;
	transactionJson["amount"] = static_cast<double>(it->second.coins) / UNITS;
	responseJson["transaction"] = transactionJson;

	response.code = 200;
	response.body = responseJson.dump();
	return response;
}

crow::response Blockchain::createTransaction(const crow::request& req) {
	auto json = nlohmann::json::parse(req.body);
	nlohmann::json responseJson;
	nlohmann::json errorJson;

	crow::response res;
	res.set_header("Content-Type", "application/json");

	auto senderPK_ = getBytes < PublicKey{}.size() > (json, "sender", errorJson, res);
	auto receiver_ = getBytes < Addr{}.size() > (json, "receiver", errorJson, res);
	auto signature_ = getBytes < Signature{}.size() > (json, "signature", errorJson, res);
	auto amount_ = getField<uint64_t>(json, "amount", errorJson, res);
	auto timestamp_ = getField<uint64_t>(json, "timestamp", errorJson, res);

	if (!senderPK_ || !receiver_ || !signature_ || !amount_ || !timestamp_) return res;

	const auto& senderPK = *senderPK_;
	const auto& receiver = *receiver_;
	const auto& signature = *signature_;
	const auto& timestamp = *timestamp_;
	const auto& amount = *amount_;

	Addr sender = computeAddress(senderPK);

	std::vector<Input> inputs;
	std::vector<UTXO> outputs;

	for (const auto& i : json["inputs"]) {
		if (utxo.find(i) == utxo.end()) {
			errorJson["error"] = "Invalid inputs";
			res.body = errorJson.dump();
			return res;
		}
		inputs.emplace_back(i);
	}
	for (const auto& o : json["outputs"]) {
		Addr addr = toBytes < Addr{}.size() > (o["address"].get<std::string>());
		uint64_t coins = (o["coins"].get<uint64_t>());
		outputs.emplace_back(addr, coins);
	}

	for (const auto& i : inputs) {
		if (mempoolInputs.find(i.getUTXOKey()) != mempoolInputs.end()) {
			Logger::reject("Inputs are already beign used");
			errorJson["error"] = "Inputs are already beign used";
			res.body = errorJson.dump();
			return res;
		}
	}

	Transaction tx{ sender, receiver, amount, inputs, outputs, timestamp };
	SignedTransaction sTransaction{ tx, senderPK, signature };

	res.code = 200;
	auto vTransaction = addTransaction(sTransaction);
	if (!vTransaction) {
		Logger::reject(vTransaction.error());
		responseJson["error"] = vTransaction.error();
		return responseJson.dump();
	}
	nlohmann::json successJson;
	Logger::log("TRANSACTION ADDED TO POOL");
	successJson["success"] = "Transaction added to pool";

	for (const auto& i : tx.inputs)
		mempoolInputs.emplace(i.getUTXOKey(), i);

	return successJson.dump();
}

// Note: Sender will always send public keys, always :D

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
	transactionHashes.emplace_back(toBytes < Hash{}.size() > (json["transactions"][0]));

	res.set_header("Content-Type", "application/json");
	for (size_t i = 1; i < json["transactions"].size(); i++) {
		const auto& t = json["transactions"][i];
		if (transactionsPool.find(t) == transactionsPool.end()) {
			res.code = 500;
			errorJson["error"] = "Invalid transaction provided";
			res.body = errorJson.dump();
			return res;
		}
		transactionHashes.emplace_back(toBytes < Hash{}.size() > (t));
	}

	Hash expectedMerkleRoot = Cryptography::computeMerkleRoot(transactionHashes);
	if (transactionHashes.size() > 1 && expectedMerkleRoot != *merkleRoot) {
		Logger::reject("B: MERKLE ROOT VERIFICATION FAILED\n");
		errorJson["error"] = "Merkle root doesn't match";
		res.code = 500;
		res.body = errorJson.dump();
		return res;
	}

	if (*previousHash != blocks.back().blockHeader.hash) {
		Logger::reject("B: PREVIOUS HASH DOESN'T MATCH\n");
		errorJson["error"] = "Previous hash doesn't match";
		res.code = 500;
		res.body = errorJson.dump();
		return res;
	}

	if (!verifyDifficulty(*hash)) {
		Logger::reject("B: HASH VERIFICATION FAILED\n");
		errorJson["error"] = "Hash should be less or equal to target";
		res.code = 500;
		res.body = errorJson.dump();
		return res;
	}

	std::vector<Transaction> blockTransactions;
	Addr dummy{};
	UTXO minerUTXO{ *minerAddress, MINER_REWARD };
	Transaction cTX{ dummy, *minerAddress, MINER_REWARD, {}, std::vector{ minerUTXO }, *timestamp };
	blockTransactions.push_back(cTX);

	for (size_t i = 1; i < transactionHashes.size(); i++) {
		const auto& t = transactionHashes[i];
		std::string txHashHex = toHex(t);

		auto it = transactionsPool.find(txHashHex);
		if (it == transactionsPool.end()) {
			Logger::reject("B: TRANSACTION NOT FOUND IN POOL\n");
			errorJson["error"] = "Transaction not found in pool: " + txHashHex;
			res.code = 404;
			res.body = errorJson.dump();
			return res;
		}
		blockTransactions.push_back(it->second);
		poolsDB.remove(it->first);
		transactionsPool.erase(it);
	}
	blocks.emplace_back(*previousHash, *hash, *timestamp, *nonce, blockTransactions);
	blocksMap[toHex(*hash)] = blocks.size() - 1;
	blocksDB.saveKey(generateBlockKey(), serializeBlock(blocks.back()));

	for (const auto& t : blockTransactions) {
		transactions.emplace(toHex(t.transaction_hash), t);
		for (const auto& i : t.inputs) {
			mempoolInputs.erase(i.getUTXOKey());
		}
		updateUTXO(t);
	}

	Logger::log("BLOCK ADDED");
	successJson["success"] = "Block been added";
	res.code = 200;
	res.body = successJson.dump();
	return res;
}

crow::response Blockchain::getBlock(const std::string& hash) {
	crow::response response;
	nlohmann::json responseJson, blockJson;
	response.set_header("Content-Type", "application/json");
	response.add_header("Access-Control-Allow-Origin", "*");
	response.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	response.add_header("Access-Control-Allow-Headers", "Content-Type");

	if (hash.size() != crypto_generichash_BYTES * 2) {
		responseJson["error"] = "Invalid block hash";
		response.code = 404;
		response.body = responseJson.dump();
		return response;
	}

	auto it = blocksMap.find(hash);
	if (it == blocksMap.end()) {
		responseJson["error"] = "Block not found";
		response.code = 404;
		response.body = responseJson.dump();
		return response;
	}

	const auto& b = blocks[it->second];
	blockJson["hash"] = hash;
	blockJson["previousHash"] = toHex(b.blockHeader.previous_hash);
	blockJson["merkleRoot"] = toHex(b.blockHeader.merkleRoot);
	blockJson["nonce"] = b.blockHeader.nonce;
	blockJson["timestamp"] = b.blockHeader.timestamp;

	auto transactionsJson = nlohmann::json::array();
	for (const auto& t : b.transactions) transactionsJson.push_back(toHex(t.transaction_hash));

	blockJson["transactions"] = transactionsJson;
	responseJson["block"] = blockJson;
	response.code = 200;
	response.body = responseJson.dump();
	return response;
}

crow::response Blockchain::getChain() {
	crow::response response;
	nlohmann::json responseJson;
	response.set_header("Content-Type", "application/json");
	response.add_header("Access-Control-Allow-Origin", "*");
	response.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	response.add_header("Access-Control-Allow-Headers", "Content-Type");

	response.code = 200;
	responseJson["tip"] = toHex(blocks.back().blockHeader.hash);
	responseJson["height"] = blocks.size() - 1;
	responseJson["difficulty"] = difficulty;
	responseJson["transactionsCount"] = transactions.size();
	responseJson["utxosCount"] = utxo.size();
	responseJson["poolCount"] = transactionsPool.size();
	response.body = responseJson.dump();
	return response;
}

crow::response Blockchain::getBlocks() {
	crow::response response;
	nlohmann::json responseJson;
	nlohmann::json blocksArray = nlohmann::json::array();
	response.set_header("Content-Type", "application/json");
	response.add_header("Access-Control-Allow-Origin", "*");
	response.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	response.add_header("Access-Control-Allow-Headers", "Content-Type");

	size_t i = 0;
	for (const auto& b : blocks) {
		nlohmann::json json;
		json["id"] = i++;
		json["hash"] = toHex(b.blockHeader.hash);
		json["previousHash"] = toHex(b.blockHeader.previous_hash);
		json["merkleRoot"] = toHex(b.blockHeader.merkleRoot);
		json["transactionCount"] = b.transactions.size();
		json["nonce"] = b.blockHeader.nonce;
		json["timestamp"] = b.blockHeader.timestamp;
		blocksArray.push_back(json);
	}
	responseJson["blocks"] = blocksArray;
	response.body = responseJson.dump();
	response.code = 200;
	return response;
}

crow::response Blockchain::getTransactions() {
	crow::response response;
	nlohmann::json responseJson;
	nlohmann::json transactionsArray = nlohmann::json::array();
	response.set_header("Content-Type", "application/json");
	response.add_header("Access-Control-Allow-Origin", "*");
	response.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	response.add_header("Access-Control-Allow-Headers", "Content-Type");

	for (const auto& t : transactions) {
		nlohmann::json json;
		nlohmann::json inputsArray = nlohmann::json::array();
		nlohmann::json outputsArray = nlohmann::json::array();

		json["transactionHash"] = t.first;
		json["sender"] = toHex(t.second.sender);
		json["receiver"] = toHex(t.second.receiver);
		json["amount"] = t.second.coins;
		json["timestamp"] = t.second.timestamp;

		for (const auto& in : t.second.inputs) {
			inputsArray.push_back(in.getUTXOKey());
		}
		for (const auto& out : t.second.outputs) {
			nlohmann::json outputJson;
			outputJson["owner"] = toHex(out.owner);
			outputJson["coins"] = out.coins;
			outputsArray.push_back(outputJson);
		}
		json["inputs"] = inputsArray;
		json["outputs"] = outputsArray;
		transactionsArray.push_back(json);
	}
	responseJson["transactions"] = transactionsArray;
	response.code = 200;
	response.body = responseJson.dump();
	return response;
}

crow::response Blockchain::getUTXOs() {
	crow::response response;
	nlohmann::json responseJson;
	nlohmann::json utxosArray = nlohmann::json::array();
	response.set_header("Content-Type", "application/json");
	response.add_header("Access-Control-Allow-Origin", "*");
	response.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	response.add_header("Access-Control-Allow-Headers", "Content-Type");

	for (const auto& u : utxo) {
		nlohmann::json utxoJson;
		utxoJson["utxoKey"] = u.first;
		utxoJson["owner"] = toHex(u.second.owner);
		utxoJson["coins"] = u.second.coins;
		utxosArray.push_back(utxoJson);
	}
	responseJson["utxos"] = utxosArray;
	response.code = 200;
	response.body = responseJson.dump();
	return response;
}

crow::response Blockchain::getPool() {
	crow::response response;
	nlohmann::json responseJson;
	nlohmann::json poolArray = nlohmann::json::array();
	response.set_header("Content-Type", "application/json");
	response.add_header("Access-Control-Allow-Origin", "*");
	response.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	response.add_header("Access-Control-Allow-Headers", "Content-Type");

	for (const auto& pt : transactionsPool) {
		nlohmann::json json;
		json["transactionHash"] = pt.first;
		json["sender"] = toHex(pt.second.sender);
		json["receiver"] = toHex(pt.second.receiver);
		json["amount"] = pt.second.coins;
		json["timestamp"] = pt.second.timestamp;
		poolArray.push_back(json);
	}
	responseJson["pool"] = poolArray;
	response.code = 200;
	response.body = responseJson.dump();
	return response;
}

void Blockchain::startServer() {
	crow::SimpleApp server;
	server.loglevel(crow::LogLevel::Info);
	CROW_ROUTE(server, "/transactions")([this]() {
		return getTransactions();
	});
	CROW_ROUTE(server, "/transactions/<string>")([this](std::string hash) {
		return getTransaction(hash);
	});
	CROW_ROUTE(server, "/blocks")([this]() {
		return getBlocks();
	});
	CROW_ROUTE(server, "/blocks/<string>")([this](std::string hash) {
		return getBlock(hash);
	});
	CROW_ROUTE(server, "/chain")([this]() {
		return getChain();
	});
	CROW_ROUTE(server, "/utxos")([this]() {
		return getUTXOs();
	});
	CROW_ROUTE(server, "/pool")([this]() {
		return getPool();
	});
	CROW_ROUTE(server, "/address/<string>")([this](std::string address) {
		Addr addr = toBytes < Addr{}.size() > (address);
		return getBalance(addr);
	});
	CROW_ROUTE(server, "/createTransaction").methods(crow::HTTPMethod::Post)([this](const crow::request& req) {
		return createTransaction(req);
	});
	CROW_ROUTE(server, "/utxo").methods(crow::HTTPMethod::Post)([this](const crow::request& req) {
		return getUTXO(req);
	});
	CROW_ROUTE(server, "/validateBlock").methods(crow::HTTPMethod::Post)([this](const crow::request& req) {
		return createBlock(req);
	});
	Logger::log("SERVER STARTED");
	server.loglevel(crow::LogLevel::Critical);
	server.port(18080).run();
}
