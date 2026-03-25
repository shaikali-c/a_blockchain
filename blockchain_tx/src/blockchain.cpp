#include <blockchain.h>

Blockchain::Blockchain() : keysDB("C:/Blockchain/Databases/keys"), utxoDB("C:/Blockchain/Databases/utxo"), transactionDB("C:/Blockchain/Databases/transactions") {

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

void Blockchain::init(const std::array<unsigned char, PUBLIC_KEY_BYTES>& owner) {
	utxo["TX_HASH:1"] = UTXO{ 100000, owner };
	saveUTXO();
}

void Blockchain::saveUTXO() {
	for (auto it = utxo.begin(); it != utxo.end(); it++) {
		utxoDB.saveKey(it->first, it->second.serialize());
	}
}

std::pair<uint64_t, std::vector<std::string>> Blockchain::findUTXO(
	const std::array<unsigned char, PUBLIC_KEY_BYTES>& owner, uint64_t amount
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
	const std::array<unsigned char, PUBLIC_KEY_BYTES>& s,
	const std::array<unsigned char, PUBLIC_KEY_BYTES>& r,
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

DBManager& Blockchain::getkeysDB() {
	return keysDB;
}