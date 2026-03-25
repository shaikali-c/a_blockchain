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
		Keys shaik{ blockchain.getkeysDB(), "shaiks" };
		Keys ali{ blockchain.getkeysDB() }, aasia{ blockchain.getkeysDB() }, ashi{ blockchain.getkeysDB() };
		//blockchain.init(shaik.publicKey());
		blockchain.listUTXO();
		blockchain.listTransactions();
	} catch(const char* err) {
		std::cout << "[LOG] Blockchain initialization failed :(\n" << err << std::endl;
	}

	return 0;
}