#include <blockchain.h>

using namespace drogon;
using Callback = std::function<void(const HttpResponsePtr&)>;

// TODO: Make the path portable
Blockchain::Blockchain(): utxoDB("utxo"), transactionDB("transactions") {

	auto utxoFuture = std::async(std::launch::async, &Blockchain::loadUTXO, this);
	auto txFuture = std::async(std::launch::async, &Blockchain::loadTransactions, this);

	utxoFuture.get();
	Logger::log("UTXO loaded");
	txFuture.get();
	Logger::log("Transactions loaded");
}

Blockchain& Blockchain::getInstance() {
	static Blockchain blockchainInstance;
	return blockchainInstance;
}

void Blockchain::init(const std::array<unsigned char, crypto_generichash_BYTES>& owner) {
	utxo["TX_HASH:1"] = UTXO{ 100000, owner};
	saveUTXO();
}

void Blockchain::saveUTXO() {
	for (auto it = utxo.begin(); it != utxo.end(); it++) {
		utxoDB.saveKey(it->first, it->second.serialize());
	}
}

std::pair<uint64_t, std::vector<std::string>> Blockchain::findUTXO(
	const std::array<unsigned char, crypto_generichash_BYTES>& owner, uint64_t amount
) {
	uint64_t total = 0;
	std::vector<std::string> collect_outputs;
	for (const auto& [txid, out] : utxo) {
		if (out.owner == owner) {
			total += out.coins;
			collect_outputs.push_back(txid);
			if (total >= amount) break;
		}
	}
	return std::make_pair(total, collect_outputs);
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

bool Blockchain::createTransaction(
	const std::array<unsigned char, crypto_generichash_BYTES>& s,
	const std::array<unsigned char, crypto_generichash_BYTES>& r,
	uint64_t amount
) {

	if (amount == 0) {
		Logger::reject("Invalid amount provided");
		return false;
	}

	auto [total, collect_outputs] = findUTXO(s, amount);

	if (total < amount) {
		Logger::reject("Not enough coins");
		return false;
	}

	for (const auto& utxo_key : collect_outputs) {
		utxo.erase(utxo_key);
		utxoDB.remove(utxo_key);
	}

	std::vector<UTXO> outputs;
	std::vector<Input> inputs;
	outputs.emplace_back(amount, r);

	if (total > amount) {
		outputs.emplace_back(total - amount, s);
	}

	Transaction tx{ s, r, amount, outputs };

	for (size_t i = 0; i < outputs.size(); i++) {
		inputs.emplace_back(tx.transaction_hash, static_cast<uint32_t>(i));
		utxo[tx.transaction_hash + ":" + std::to_string(i)] = outputs[i];
	}

	tx.inputs = inputs;
	transactions.emplace(tx.transaction_hash, tx);

	saveUTXO();
	transactionDB.saveKey(tx.transaction_hash, tx.serialize());
	Logger::log("Transaction successfully created");
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

void Blockchain::startServer() {
	setupRoutes();
	drogon::app().addListener("0.0.0.0", 8080);
	std::filesystem::create_directories(LOGS_FOLDER);
	drogon::app().setLogPath("logs");
	drogon::app().run();
}

void Blockchain::setupRoutes() {
	_getTransaction();
	_getTransactions();
	_createTransaction();
}

void Blockchain::_getTransactions() {

	app().registerHandler("/transactions", [this](const HttpRequestPtr& req, Callback&& callback) {
		Json::Value transacitons_list(Json::arrayValue);
		for (const auto& pair : transactions) {
			Json::Value tx;
			tx["transaction_hash"] = pair.first;
			tx["sender"] = toHex(pair.second.sender.data(), pair.second.sender.size());
			tx["receiver"] = toHex(pair.second.receiver.data(), pair.second.receiver.size());
			tx["amount"] = pair.second.coins;
			tx["time"] = pair.second.timestamp;
			transacitons_list.append(tx);
		}
		callback(HttpResponse::newHttpJsonResponse(transacitons_list));
	}, { Get });

}

void Blockchain::_getTransaction() {
	app().registerHandler("/transaction/{tx_id}", [this](const HttpRequestPtr& req, Callback&& callback, std::string tx_id) {
		LOG_INFO << "Request Came";
		auto it = transactions.find(tx_id);
		Json::Value tx;
		if (it != transactions.end()) {
			tx["transaction_hash"] = it->first;
			tx["sender"] = toHex(it->second.sender.data(), it->second.sender.size());
			tx["receiver"] = toHex(it->second.receiver.data(), it->second.receiver.size());
			tx["amount"] = it->second.coins;
			tx["time"] = it->second.timestamp;
			callback(HttpResponse::newHttpJsonResponse(tx));
		}
		else {
			tx["error"] = "TRANSACTION_NOT_FOUND";
			callback(HttpResponse::newHttpJsonResponse(tx));
		}
	}, { Get });
}

void Blockchain::_createTransaction() {
	app().registerHandler("/create_transaction", [this](const HttpRequestPtr& req, Callback&& callback) {

		auto &jsonBody = req->getJsonObject();
		Json::Value response;

		for (const auto& tx : *jsonBody) {
			if (!tx.isObject()) continue;

			std::string sender_serialize = tx["sender"].asString();
			std::string receiver_serialize = tx["receiver"].asString();
			uint64_t amount = tx["amount"].asUInt64();

			std::array<unsigned char, crypto_generichash_BYTES> sender;
			std::array<unsigned char, crypto_generichash_BYTES> receiver;

			toBytes(sender_serialize, sender.data(), sender.size());
			toBytes(receiver_serialize, receiver.data(), receiver.size());

			this->createTransaction(sender, receiver, amount);
		}

		callback(HttpResponse::newHttpJsonResponse(response));
		}, { Post });
}