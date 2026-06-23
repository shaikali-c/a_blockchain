#include "pch.h"
#include "blockchain.h"
#include "databaseManager.h"
#include "logger.h"
#include <sodium.h>

int main()
{
	if (sodium_init() < 0) {
		Logger::error("Library initilization failed :(");
	}
	try {
		Blockchain& blockchain = Blockchain::getInstance();
		blockchain.startServer();
	} catch(...) {
		Logger::error("Blockchain initialization failed :(");
	}
	std::cin.get();
	return 0;
}