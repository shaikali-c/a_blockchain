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
			
		KeysManager shaik, ali;
		std::cout << "Shaik: " << Common::toHex(shaik.getAddress()) << "\nAli: " << Common::toHex(ali.getAddress()) << "\n";
		blockchain.spareCoins(shaik.getAddress());
		UTXOResult result = blockchain.getUTXO(shaik.getAddress(), 50);
		SignedTransaction tx = shaik.createTransaction(result.inputs, result.total, ali.getAddress(), 50);
		blockchain.addTransaction(tx);
		blockchain.listUTXO();
		blockchain.listTransactions();

	} catch(...) {
		Logger::error("Blockchain initialization failed :(");
	}
	std::cin.get();

	return 0;
}
