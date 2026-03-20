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

	Blockchain blockchain;

	Keys shaik{ blockchain.getkeysDB(), "shaik"};
	Keys ali{ blockchain.getkeysDB(), "ali" }, aasia{ blockchain.getkeysDB() }, rashed{ blockchain.getkeysDB() };

	//blockchain.init(shaik.publicKey());


	blockchain.listUTXO();
	//blockchain.listTransactions();


	return 0;
}