#include <blockchain.h>

Blockchain::Blockchain() : keysDB("keys"), utxoDB("utxo") {
	loadUTXO();
}

void Blockchain::init(const std::array<unsigned char, PUBLIC_KEY_BYTES>& owner) {
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

bool Blockchain::createTransaction(
	const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& s,
	const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& r,
	uint64_t amount
) {

	if (amount == 0) {
		std::cout << "Transaction amount must be positive\n";
		return false;
	}


	std::vector<std::string> collect_outputs;
	uint64_t total = 0;

	for (auto& output : utxo) {
		if (output.second.owner == s) {
			total += output.second.coins;
			collect_outputs.push_back(output.first);
		}
		if (total >= amount) break;
	}

	if (total < amount) {
		std::cout << "Not Enough Coins!\n";
		return false;
	}

	for (const auto& utxo_key : collect_outputs) {
		utxo.erase(utxo_key);
		utxoDB.remove(utxo_key);
	}

	std::vector<UTXO> outputs;
	outputs.emplace_back(amount, r);

	if (total > amount) {
		outputs.emplace_back(total - amount, s);
	}

	Transaction tx{ s, r, amount, outputs };
	std::vector<Input> inputs;

	for (size_t i = 0; i < outputs.size(); i++) {
		std::string utxo_key = tx.transaction_hash + ":" + std::to_string(i);
		inputs.emplace_back(tx.transaction_hash, static_cast<uint32_t>(i));
		utxo[utxo_key] = outputs[i];
	}

	for (auto it = utxo.begin(); it != utxo.end(); it++) {
		std::cout << it->first << "\n";
		utxoDB.saveKey(it->first, it->second.serialize());
	}



	return true;

}

DBManager& Blockchain::getkeysDB() {
	return keysDB;
}