#include "pch.h"
#include "blockchain.h"
#include "keys.h"
#include "common.h"
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
		DBManager keysDB{ "./keys" };

		//Keys shaik, ali;
		//keysDB.saveKey("shaik", shaik.serializeKeys());
		//keysDB.saveKey("ali", ali.serializeKeys());

		Keys shaik{ keysDB.loadKey("shaik") }, ali{ keysDB.loadKey("ali") };

		//blockchain.init(shaik.addr);
		blockchain.createTransaction(shaik.addr, ali.addr, 1);

		blockchain.listUTXO();
		blockchain.listTransactions();
		blockchain.startServer();

	} catch(...) {
		Logger::error("Blockchain initialization failed :(");
	}
	std::cin.get();

	return 0;
}
