#include <blockchain.h>

using namespace drogon;

// TODO: Make the path portable
Blockchain::Blockchain() : utxoDB("utxo"), transactionDB("transactions"), height{ 0 } {
	loadTransactions();
	LOG_INFO << "Transactions loaded";
}

Blockchain& Blockchain::getInstance() {
	static Blockchain blockchainInstance;
	return blockchainInstance;
}

void Blockchain::loadTransactions() {
	leveldb::Iterator* it = transactionDB.db->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string value = it->value().ToString();
		Transaction transaction{ value };
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
	transactionDB.saveKey(Common::toHex(signedTransaction.transaction.transaction_hash), signedTransaction.transaction.serializeTransaction());
	LOG_INFO << "TRANSACTION CREATED";
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
	std::cout << transactions_table << "\n";
}

void Blockchain::listUTXO() const {
	tabulate::Table utxo_table;
	utxo_table.add_row({ "UTXO Key", "Owner", "Coins" });
	for (const auto& [utxo_key, out] : utxo) {
		utxo_table.add_row({ utxo_key, Common::toHex(out.owner), std::to_string(out.coins)});
	}
	std::cout << utxo_table << "\n";45
}