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
		
		DBManager keysDB{ "keys" };
		/*KeysManager shaik, ali;
		keysDB.saveKey("shaik", shaik.serializeKeys());*/
		KeysManager shaik{ keysDB.loadKey("shaik") }, ali;
		std::cout << "Shaik: " << Common::toHex(shaik.getAddress()) << "\nAli: " << Common::toHex(ali.getAddress()) << "\n";

		//blockchain.spareCoins(shaik.getAddress());
		//UTXOResult result = blockchain.getUTXO(shaik.getAddress(), 5);
		//SignedTransaction tx = shaik.createTransaction(result.inputs, result.total, ali.getAddress(), 5);
		//blockchain.addTransaction(tx);
		/*UTXO utxo{ shaik.getAddress(), 500 };
		Hash pHashDummy{};
		pHashDummy.fill(0);
		std::array<unsigned char, 20> dummy{};
		Transaction tx{ shaik.getAddress(), shaik.getAddress(), 500, {}, {utxo}};

		Miner miner{ pHashDummy, {tx} };
		Block block = miner.mine();
		std::cout << "\n\nHash: " << Common::toHex(block.blockHeader.hash) << "\nPrevious hash: " << Common::toHex(block.blockHeader.previous_hash) << "\nNonce: " << block.blockHeader.nonce << "\nTimestamp: " << block.blockHeader.timestamp << "\nMerkle root: " << Common::toHex(block.blockHeader.merkleRoot) << "\n";*/


		/*blockchain.listUTXO();
		blockchain.listTransactions();*/
		blockchain.listBlocks();

	} catch(...) {
		Logger::error("Blockchain initialization failed :(");
	}
	std::cin.get();

	return 0;
}

//Shaik: b4720462ac198c6e6a55a89de9445498a64406aa
//Ali : 85219450ec69da7548388da9348339059536e4b3
//Timestamp tx : 1780775044370156300
//
//
//Hash : 00a92349100089d140982ef16742911adc981e3439b0cb15fcccba99320b316b
//Previous hash : 0000000000000000000000000000000000000000000000000000000000000000
//Nonce : 29
//Timestamp : 1780775044373228000
//Merkle root : e5d79d482d0e7ab7cbf67d56e59eaebc04893a3cf57766d97c311f2a9c28db2e
//+ ---------- + ------ - +------ - +
