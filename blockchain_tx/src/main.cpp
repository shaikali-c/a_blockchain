#include "pch.h"
#include "blockchain.h"
#include "keys.h"
#include "databaseManager.h"
#include "logger.h"
#include <sodium.h>

int main()
{

	if (sodium_init() < 0) {
		Logger::reject("Library initilization failed :(");
	}
	try {
		Blockchain& blockchain = Blockchain::getInstance();
		Logger::log("Blockchain initialized :)");
		DBManager keysDB{ "./keys" };
			
		//Keys shaik, ali;
		//keysDB.saveKey("shaik", shaik.serializeKeys());
		//keysDB.saveKey("ali", ali.serializeKeys());
		//blockchain.init(shaik.addr);
		
		Keys shaik{ keysDB.loadKey("shaik") }, ali{ keysDB.loadKey("ali") };

		uint64_t amount = 5000;

		//std::pair<std::vector<Input>, uint64_t> collect = blockchain.getUTXO(shaik.addr, amount);
		//std::pair<Transaction, std::array<unsigned char, crypto_sign_BYTES>> tx_and_signature = shaik.createTransaction(ali.addr, amount, collect.first, collect.second);

		//blockchain.verifyTX(tx_and_signature.first, shaik.publicKey(), tx_and_signature.second);
		blockchain.listUTXO();
		blockchain.listTransactions();
		blockchain.startServer();

	} catch(...) {
		Logger::error("Blockchain initialization failed :(");
	}
	std::cin.get();

	return 0;
}
