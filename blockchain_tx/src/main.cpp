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
		DBManager keysDB{ "C:/Blockchain/Databases/keys" };
		Keys shaik{ keysDB.loadKey("shaiks_keys") }, ali{ keysDB.loadKey("ali_keys") }, aasia{ keysDB.loadKey("aasias_keys") };
		shaik.printKeys();
		Logger::log(toHex(shaik.addr.data(), shaik.addr.size()));
		blockchain.createTransaction(shaik.addr, aasia.addr, 500);
		blockchain.listUTXO();
		blockchain.listTransactions();
	} catch(...) {
		Logger::log("Blockchain initialization failed :(\n");
	}

	std::cin.get();

	return 0;
}