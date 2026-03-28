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

		Keys shaik{ keysDB.loadKey("shaik") }, ali{ keysDB.loadKey("ali") };

		blockchain.init(shaik.addr);
		blockchain.createTransaction(shaik.addr, ali.addr, 500);
		blockchain.listUTXO();
		blockchain.listTransactions();
		blockchain._getTransactions();
		blockchain.startServer();

	} catch(...) {
		Logger::error("Blockchain initialization failed :(\n");
	}
	std::cin.get();

	return 0;
}
