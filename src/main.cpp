#include "pch.h"
#include "blockchain.h"
#include "keys.h"
#include "miner.h"
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
		blockchain.listUTXO();
		blockchain.listBlocks();
		blockchain.listPoolTransactions();
		blockchain.listTransactions();
		blockchain.startServer();
	} catch(...) {
		Logger::error("Blockchain initialization failed :(");
	}
	std::cin.get();
	return 0;
}