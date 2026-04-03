#include <blockchain.h>

using namespace drogon;
using Callback = std::function<void(const HttpResponsePtr&)>;

// TODO: Make the path portable
Blockchain::Blockchain(): utxoDB("utxo"), transactionDB("transactions") {

	loadUTXO();
	Logger::log("UTXO loaded");
	loadTransactions();
	Logger::log("Transactions loaded");
}

Blockchain& Blockchain::getInstance() {
	static Blockchain blockchainInstance;
	return blockchainInstance;
}

void Blockchain::init(const std::array<unsigned char, crypto_generichash_BYTES>& owner) {
	Input input{ "TX_HASH", 1 };
	utxo[input.getUTXOKey()] = UTXO{100000, owner};
	saveUTXO();
}

void Blockchain::saveUTXO() {
	for (auto it = utxo.begin(); it != utxo.end(); it++) {
		utxoDB.saveKey(it->first, it->second.serialize());
	}
}

std::pair<std::vector<Input>, uint64_t> Blockchain::getUTXO(const std::array<unsigned char, crypto_generichash_BYTES>& addr, uint64_t coins) {
	std::vector<Input> inputs;
	uint64_t total = 0;
	for (const auto& [txid, out] : utxo) {
		if (out.owner == addr) {
			total += out.coins;
			size_t pos = txid.find(':');
			std::string tx_hash = txid.substr(0, pos);
			uint32_t output_index = std::stoul(txid.substr(pos + 1));
			inputs.emplace_back(tx_hash, output_index);
			if (total >= coins) break;
		}
	}
	return std::make_pair(inputs, total);
}

void Blockchain::loadUTXO() {
	std::unique_ptr<leveldb::Iterator> it(utxoDB.db->NewIterator(leveldb::ReadOptions()));

	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string key = it->key().ToString();
		std::string value = it->value().ToString();
		utxo[key] = UTXO{ value };
	}

	if (!it->status().ok()) {
		std::cerr << "Iterator error: " << it->status().ToString() << "\n";
	}
}
void Blockchain::loadTransactions() {
	std::unique_ptr<leveldb::Iterator> it(transactionDB.db->NewIterator(leveldb::ReadOptions()));

	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string key = it->key().ToString();
		std::string value = it->value().ToString();
		Transaction tx;
		tx.deserialize(value);
		transactions[key] = tx;
	}

	if (!it->status().ok()) {
		std::cerr << "Iterator error: " << it->status().ToString() << "\n";
	}
}

bool Blockchain::verifySignature(
	const std::array<unsigned char, crypto_sign_BYTES>& signature,
	const std::array<unsigned char, crypto_generichash_BYTES>& msg,
	const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& publicKey
) {
	if (crypto_sign_verify_detached(signature.data(), reinterpret_cast<const unsigned char*>(msg.data()), msg.size(), publicKey.data()) != 0) {
		Logger::reject("SIGNATURE FAILED");
		return false;
	}
	return true;
}

bool Blockchain::verifyTX(
	Transaction& tx,
	const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& publicKey,
	const std::array<unsigned char, crypto_sign_BYTES>& signature
) {

	if (tx.isCoinbase) {
		return true;
	}

	if (tx.inputs.empty()) {
		Logger::reject("NO UTXOs FOUND");
		return false;
	}

	for (const auto& in : tx.inputs) {
		auto it = utxo.find(in.getUTXOKey());
		if (it == utxo.end()) {
			Logger::reject("INVALID INPUT");
			return false;
		}
		if (it->second.owner != _hashBytes(publicKey)) {
			Logger::reject("OWNERSHIP FAILED");
			return false;
		}
		if (!verifySignature(signature, tx.transaction_hash_bytes, publicKey)) {
			Logger::reject("SIGNATURE FAILED");
			return false;
		}
	}
	

	for (const auto& utxo_key : tx.inputs) {
		utxo.erase(utxo_key.getUTXOKey());
		utxoDB.remove(utxo_key.getUTXOKey());
	}
	
	for (size_t i = 0; i < tx.outputs.size(); i++) {
		utxo[tx.transaction_hash + ":" + std::to_string(i)] = tx.outputs[i];
	}
	
	transactions.emplace(tx.transaction_hash, tx);
	transactionDB.saveKey(tx.transaction_hash, tx.serialize());
	saveUTXO();

	Logger::log("SUCCESS");
	return true;
}

void Blockchain::listTransactions() const {
	tabulate::Table transactions_table;
	transactions_table.add_row({ "Transaction Hash", "Sender", "Receiver", "Coins", "Timestamp"});
	for (const auto& [txid, tx] : transactions) {
		transactions_table.add_row({tx.transaction_hash, toHex(tx.sender.data(), tx.sender.size()), toHex(tx.receiver.data(), tx.receiver.size()), std::to_string(tx.coins), std::to_string(tx.timestamp)});
	}
	std::cout << transactions_table << "\n";

}

void Blockchain::listUTXO() const {
	tabulate::Table utxo_table;
	utxo_table.add_row({ "UTXO Key", "Owner", "Coins" });
	for (const auto& [utxo_key, out] : utxo) {
		utxo_table.add_row({ utxo_key, toHex(out.owner.data(), out.owner.size()), std::to_string(out.coins)});
	}
	std::cout << utxo_table << "\n";
}

void Blockchain::getUTXOAPI() {
	app().registerHandler("/utxo",[this](const HttpRequestPtr& req, Callback&& callback) {

		auto json = req->getJsonObject();
		Json::Value root;
		if (!json) {
			root["error"] = "INVALID_JSON";
		}
		else {
			Json::Value utxos(Json::arrayValue);
			uint64_t total = 0;	

			std::string addr_hex = (*json)["addr"].asString();
			uint64_t amount = (*json)["amount"].asInt64();

			std::array<unsigned char, crypto_generichash_BYTES> addr;
			toBytes(addr_hex, addr.data(), addr.size());

			for (const auto& [utxo_key, out] : utxo) {
				Json::Value utxo_;
				if (out.owner == addr) {
					utxo_["utxo_key"] = utxo_key;
					total += out.coins;
					utxos.append(utxo_);
					if (total >= amount) break;
				}
			
			}
			if (total < amount) {
				root["error"] = "NOT_ENOUGH_COINS";
			} else {
				root["utxos"] = utxos;
			}
		}
		auto response = HttpResponse::newHttpJsonResponse(root);
		callback(response);
		}, { Post });
}

void Blockchain::getTransactionsAPI() {
	app().registerHandler("/transactions", [this](const HttpRequestPtr& req, Callback&& callback) {
		Json::Value root;
		if (transactions.empty()) {
			root["error"] = "NO_TRANSACTIONS";
		} else {
			Json::Value transactionsJson(Json::arrayValue);
			for (const auto& [tx_hash, tx] : transactions) {
				Json::Value transactionJson;
				transactionJson["transaction_hash"] = tx_hash;
				transactionJson["sender"] = toHex(tx.sender.data(), tx.sender.size());
				transactionJson["receiver"] = toHex(tx.receiver.data(), tx.receiver.size());
				transactionJson["amount"] = tx.coins;
				transactionJson["timestamp"] = tx.timestamp;
				transactionsJson.append(transactionJson);
			}
			root["transactions"] = transactionsJson;
		}
		auto response = HttpResponse::newHttpJsonResponse(root);
		callback(response);
		}, { Get });
}

void Blockchain::getTransactionAPI() {
	app().registerHandler("/transactions/{tx_hash}", [this](const HttpRequestPtr& req, Callback&& callback, const std::string& tx_hash) {
		auto it = transactions.find(tx_hash);
		Json::Value root;
		if (it != transactions.end()) {
			Json::Value transactionJson;
			transactionJson["transaction_hash"] = it->first;
			transactionJson["sender"] = toHex(it->second.sender.data(), it->second.sender.size());
			transactionJson["receiver"] = toHex(it->second.receiver.data(), it->second.receiver.size());
			transactionJson["amount"] = it->second.coins;
			transactionJson["timestamp"] = it->second.timestamp;
			root["transaction"] = transactionJson;
		} else {
			root["error"] = "TRANSACTION_NOT_FOUND";
		}
		auto response = HttpResponse::newHttpJsonResponse(root);
		callback(response);
		}, { Get });
}

void Blockchain::setupRoutes() {
	getTransactionAPI();
	getTransactionsAPI();
	getUTXOAPI();
}

void Blockchain::startServer() {
	setupRoutes();
	app().addListener("0.0.0.0", 8080);
	app().run();
}