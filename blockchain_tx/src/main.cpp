#include <pch.h>
#include <blockchain.h>
#include <keys.h>
#include <sodium.h>
#include <common.h>

int main()
{
	if (sodium_init() < 0) {
		std::cout << ":(";
	}

	try {
		Blockchain& blockchain = Blockchain::getInstance();
		Keys shaik{ blockchain.getkeysDB(), "shaik" };
		Keys ali{ blockchain.getkeysDB(), "ali" }, aasia{ blockchain.getkeysDB() }, rashed{ blockchain.getkeysDB() };

		//blockchain.init(shaik.publicKey());

		blockchain.createTransaction(shaik.publicKey(), rashed.publicKey(), 100);

		blockchain.listUTXO();
		blockchain.listTransactions();
	} catch(const char* err) {
		std::cout << "[LOG] Blockchain initialization failed :(\n" << err << std::endl;
	}

	return 0;
}