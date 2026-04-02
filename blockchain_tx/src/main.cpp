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
		
		Keys shaik{ keysDB.loadKey("shaik") }, ali{ keysDB.loadKey("ali") };

		blockchain.init(shaik.addr);

		std::pair<std::vector<Input>, uint64_t> collect = blockchain.getUTXO(shaik.addr, 10);
		std::pair<Transaction, std::array<unsigned char, crypto_sign_BYTES>> tx_and_signature = shaik.createTransaction(ali.addr, 5000, collect.first, collect.second);

		blockchain.verifyTX(tx_and_signature.first, shaik.publicKey(), tx_and_signature.second);
		blockchain.listUTXO();
		blockchain.listTransactions();

	} catch(...) {
		Logger::error("Blockchain initialization failed :(");
	}
	std::cin.get();

	return 0;
}
