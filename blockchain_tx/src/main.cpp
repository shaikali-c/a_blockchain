#include <pch.h>
#include <blockchain.h>
#include <keys.h>
#include <sodium.h>


using namespace std;


int main()
{
	if (sodium_init() < 0) {
		std::cout << ":(";
	}

	Blockchain blockchain;

	Keys shaik{blockchain.getkeysDB(), "shaik"};
	Keys ali{blockchain.getkeysDB(), "ali"};

	blockchain.createTransaction(ali.publicKey(), shaik.publicKey(), 50);

	return 0;
}