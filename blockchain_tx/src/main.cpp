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
		
		DBManager keysDB{ "keys" };
		//KeysManager shaik, ali;
		//keysDB.saveKey("shaik", shaik.serializeKeys());
		KeysManager shaik{ keysDB.loadKey("shaik") }, ali;
		std::cout << "Shaik: " << Common::toHex(shaik.getAddress()) << "\nAli: " << Common::toHex(ali.getAddress()) << "\n";

		//blockchain.spareCoins(shaik.getAddress());
		for (uint32_t i = 0; i < 5; i++) {
			UTXOResult result = blockchain.getUTXO(shaik.getAddress(), 5);
			SignedTransaction tx = shaik.createTransaction(result.inputs, result.total, ali.getAddress(), 5);
			blockchain.addTransaction(tx);
		}

		blockchain.listUTXO();
		blockchain.listTransactions();

	} catch(...) {
		Logger::error("Blockchain initialization failed :(");
	}
	std::cin.get();

	return 0;
}
