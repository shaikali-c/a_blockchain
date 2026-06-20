	#include <blockchain.h>

	// TODO: Make the path portable
	// TODO: Make UTXO find much faster by maintaining another set with sorted coins
	// TODO: Make difficulty adjusted based on blocks been mined every minute

	Blockchain::Blockchain() : blocksDB("blocks"), poolsDB("pool"), height{0}, difficulty(1) {
		loadBlocks();
		loadPoolTransactions();
		if (blocks.empty()) createGenesisBlock();
		buildTarget();
	}

	void Blockchain::createGenesisBlock() {
		Hash previousHash{};
		previousHash.fill(0x00);
		Hash hash = Common::toBytes < Hash{}.size() > ("00007f009a96c42684187a363b47504d5461b99852853670d4edd3adc4ea777e");
		Hash mHash = Common::toBytes < Hash{}.size() > ("22e89138181eeff1e1b80dd0aa5467b47fa65fb3036bb835f6de0d3917ba8efc");;

		Addr shaik = Common::toBytes < Addr{}.size() >("f45a20e043b01f65638a46831ce79b8fec3f6737");

		UTXO utxo{ shaik, GENESIS_REWARD };
		Transaction tx{ shaik, utxo.owner, utxo.coins, {}, {utxo}, 1781545365 };
		uint64_t nonce = 31496, timestamp = 1781545365; // These values were mined and hardcoded into the genesis block to ensure all nodes share the same chain origin

		Block gBlock{ previousHash, hash, timestamp , nonce, {tx} };
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

	void Blockchain::loadBlocks() {
		std::unique_ptr<leveldb::Iterator> it(
			blocksDB.db->NewIterator(leveldb::ReadOptions())
		);
		for (it->SeekToFirst(); it->Valid(); it->Next()) {
			std::string value = it->value().ToString();
			Block block = deserializeBlock(value);
			for (const auto& t : block.transactions) {
				transactions.emplace(Common::toHex(t.transaction_hash), t);
				updateUTXO(t);
			}
			blocks.push_back(block);
			auto hashHex = Common::toHex(block.blockHeader.hash);
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
			Transaction tx{value};
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
			utxoKey.append(Common::toHex(transaction.transaction_hash));
			utxoKey.push_back(':');
			utxoKey.append(std::to_string(i));
			utxo.emplace(utxoKey, transaction.outputs[i]);
		}
	}

	std::expected<void, std::string> Blockchain::addTransaction(const SignedTransaction& signedTransaction) {
		if(signedTransaction.transaction.coins <= 0) return std::unexpected("Invalid transaction amount");
		if (!verifyTransaction(signedTransaction.transaction)) return std::unexpected("Transaction verification failed");
		if (!verifySignature(signedTransaction)) return std::unexpected("Signature verification failed");

		auto transactionHashHex = Common::toHex(signedTransaction.transaction.transaction_hash);
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
				transactions_table.add_row({ Common::toHex(tx.transaction_hash), Common::toHex(tx.sender), Common::toHex(tx.receiver), std::to_string(tx.coins), std::to_string(tx.timestamp) });
			}
			std::cout << transactions_table << "\n";
		}
	}

	void Blockchain::listPoolTransactions() const {
		if (!transactionsPool.empty()) {
			tabulate::Table transactions_table;
			transactions_table.add_row({ "Pool Hash", "Sender", "Receiver", "Coins", "Timestamp" });
			for (const auto& [txid, tx] : transactionsPool) {
				transactions_table.add_row({ Common::toHex(tx.transaction_hash), Common::toHex(tx.sender), Common::toHex(tx.receiver), std::to_string(tx.coins), std::to_string(tx.timestamp) });
			}
			std::cout << transactions_table << "\n";
		}

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
			utxo_table.add_row({ utxo_key, Common::toHex(out.owner), std::to_string(out.coins / UNITS) });
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
				utxoJson["utxoKey"] = Common::toHex(input.transaction_hash);
				utxoJson["outputIndex"] = input.output_index;
				json["utxos"].push_back(utxoJson);
				c += out.coins;
				if (c >= coins) break;
			}
		}
		json["coins"] = c;
		crow::response res;
		res.set_header("Content-Type", "application/json");
		res.code = 200;
		res.body = json.dump();
		return res;
	}

	crow::response Blockchain::getTransaction(const std::string& hash) {
		nlohmann::json responseJson;
		crow::response res;
		res.set_header("Content-Type", "application/json");

		auto it = transactions.find(hash);
		if (it == transactions.end()) {
			responseJson["error"] = "Transaction not found";
			res.code = 404;
			res.body = responseJson.dump();
			return res;
		}
		responseJson["transactionHash"] = hash;
		responseJson["sender"] = Common::toHex(it->second.sender);
		responseJson["receiver"] = Common::toHex(it->second.receiver);
		responseJson["timestamp"] = it->second.timestamp;
		responseJson["amount"] = static_cast<double>(it->second.coins) / UNITS;
	
		res.body = responseJson.dump();
		return res;
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

		if (!senderPK_|| !receiver_|| !signature_ || !amount_ || !timestamp_) return res;

		const auto& senderPK = *senderPK_;
		const auto& receiver = *receiver_;
		const auto& signature = *signature_;
		const auto& timestamp = *timestamp_;
		const auto& amount = *amount_;

		Addr sender = Common::computeAddress(senderPK);

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
			Addr addr = Common::toBytes < Addr{}.size() > (o["address"].get<std::string>());
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
		transactionHashes.emplace_back(Common::toBytes < Hash{}.size() > (json["transactions"][0]));

		res.set_header("Content-Type", "application/json");
		for (size_t i = 1; i < json["transactions"].size(); i++) {
			const auto& t = json["transactions"][i];
			if (transactionsPool.find(t) == transactionsPool.end()) {
				res.code = 500;
				errorJson["error"] = "Invalid transaction provided";
				res.body = errorJson.dump();
				return res;
			}
			transactionHashes.emplace_back(Common::toBytes < Hash{}.size() > (t));
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

		UTXO minerUTXO{ *minerAddress, MINER_REWARD };
		Transaction tx{ *minerAddress, MINER_REWARD, {minerUTXO} };

		blockTransactions.emplace_back(tx);
		for (size_t i = 1; i < transactionHashes.size(); i++) {
			const auto& t = transactionHashes[i];
			std::string txHashHex = Common::toHex(t);

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
		Block block{ *previousHash, *hash, *timestamp, *nonce, blockTransactions };
		blocks.push_back(block);

		auto hashHex = json["hash"].get<std::string>();
		blocksMap[hashHex] = blocks.size() - 1;
		blocksDB.saveKey(generateBlockKey(), serializeBlock(blocks.back()));

		for (const auto& t : blockTransactions) {
			transactions.emplace(Common::toHex(t.transaction_hash), t);
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
		nlohmann::json responseJson;
		response.set_header("Content-Type", "application/json");

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

		responseJson["hash"] = hash;
		responseJson["previousHash"] = Common::toHex(b.blockHeader.previous_hash);
		responseJson["merkleRoot"] = Common::toHex(b.blockHeader.merkleRoot);
		responseJson["nonce"] = b.blockHeader.nonce;
		responseJson["timestamp"] = b.blockHeader.timestamp;

		auto transactionsJson = nlohmann::json::array();
		for (const auto& t : b.transactions) transactionsJson.push_back(Common::toHex(t.transaction_hash));

		responseJson["transactions"] = transactionsJson;
		response.code = 200;
		response.body = responseJson.dump();
		return response;
	}

	crow::response Blockchain::getChain() {
		crow::response response;
		nlohmann::json responseJson;
		response.set_header("Content-Type", "application/json");
		response.code = 200;
		responseJson["tip"] = Common::toHex(blocks.back().blockHeader.hash);
		responseJson["height"] = blocks.size() - 1;
		responseJson["difficulty"] = difficulty;
		responseJson["transactionsCount"] = transactions.size();
		responseJson["utxosCount"] = utxo.size();
		responseJson["poolCount"] = transactionsPool.size();
		responseJson["supply"] = transactionsPool.size();
		response.body = responseJson.dump();
		return response;
	}

	void Blockchain::startServer() {
		crow::SimpleApp server;
		server.loglevel(crow::LogLevel::Info);
		CROW_ROUTE(server, "/transactions/<string>")([this](std::string hash) {
			return getTransaction(hash);
		});
		CROW_ROUTE(server, "/blocks/<string>")([this](std::string hash) {
			return getBlock(hash);
		});
		CROW_ROUTE(server, "/chain")([this]() {
			return getChain();
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
