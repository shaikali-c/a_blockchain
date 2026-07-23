#include "axis/blockchain/blockchain.h"
#include "axis/core/logger.h"
#include <sodium.h>

int main()
{
	if (sodium_init() < 0) {
		Logger::error("Library initilization failed :(");
	}
	try {
		Blockchain& blockchain = Blockchain::getInstance();
		blockchain.setupConnection();
	} catch(...) {
		Logger::error("Blockchain initialization failed :(");
	}
	std::cin.get();
	return 0;
}
