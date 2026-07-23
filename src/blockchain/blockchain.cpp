
#include "axis/blockchain/block.h"
#include "axis/blockchain/blockchain.h"
#include "axis/core/common.h"
#include "axis/core/pretty_print.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

// TODO: Make the path portable
// TODO: Make UTXO find much faster by maintaining another set with sorted coins
// TODO: Make difficulty adjusted based on blocks been mined every minute

asio::io_context context;
asio::ip::tcp::acceptor acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 9618));

Blockchain::Blockchain() : blocksDB("blocks"), poolsDB("pool"), height{ 0 }, difficulty(3) {
	loadBlocks();
	loadPoolTransactions();
	if (blocks.empty()) createGenesisBlock();
	Addr shaik = toBytes < Addr{}.size() > ("882bf725d9458874fcd74c0467df826c47eb0fac");
	utxo.emplace("00007f009a96c42684187a363b47504d5461b99852853670d4edd3adc4ea777e:20", UTXO{ shaik, GENESIS_REWARD });
	buildTarget();
}

void Blockchain::createGenesisBlock() {
	Hash previousHash{};
	previousHash.fill(0x00);
	Hash hash = toBytes < Hash{}.size() > ("00007f009a96c42684187a363b47504d5461b99852853670d4edd3adc4ea777e");
	Hash mHash = toBytes < Hash{}.size() > ("22e89138181eeff1e1b80dd0aa5467b47fa65fb3036bb835f6de0d3917ba8efc");;
	Addr shaik = toBytes < Addr{}.size() > ("f45a20e043b01f65638a46831ce79b8fec3f6737");

	UTXO utxo{ shaik, GENESIS_REWARD };
	Transaction tx{ shaik, utxo.owner, utxo.coins, {}, {utxo}, 1781545365 };
	transactions.emplace(toHex(tx.transaction_hash), tx);
	uint64_t nonce = 31496, timestamp = 1781545365; // These values were mined and hardcoded into the genesis block to ensure all nodes share the same chain origin

	Block gBlock{ previousHash, hash, timestamp , nonce, std::vector{tx} };
	gBlock.blockHeader.merkleRoot = mHash;
	blocks.push_back(gBlock);
	blocksDB.saveKey(generateBlockKey(), serializeBlock(gBlock));
	updateUTXO(tx);
}

Blockchain& Blockchain::getInstance() {
	static Blockchain blockchainInstance;
	return blockchainInstance;
}

std::string Blockchain::generateBlockKey() {
	uint64_t height = blocks.size();
	std::stringstream ss;
	ss << std::setw(10) << std::setfill('0') << height;
	std::string key = ss.str();
	return key;
}

Hash Blockchain::buildTarget() {
	Hash t{};
	t.fill(0xff);
	for (int i = 0; i < difficulty; i++) t[i] = 0x00;
	target = t;
	return t;
}

void Blockchain::loadBlocks() {
	std::unique_ptr<leveldb::Iterator> it(
		blocksDB.db->NewIterator(leveldb::ReadOptions())
	);
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string value = it->value().ToString();
		Block block{ value };
		std::cout << block << "\n";
		for (const auto& t : block.transactions) {
			transactions.emplace(toHex(t.transaction_hash), t);
			updateUTXO(t);
		}
		blocks.push_back(block);
		auto hashHex = toHex(block.blockHeader.hash);
		blocksMap[hashHex] = blocks.size() - 1;
	}

	if (!it->status().ok()) {
		std::cerr << "Iterator error: " << it->status().ToString() << std::endl;
	}
	Logger::log("BLOCKS LOADED");
}

void Blockchain::loadPoolTransactions() {
	std::unique_ptr<leveldb::Iterator> it(
		poolsDB.db->NewIterator(leveldb::ReadOptions())
	);
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		std::string key = it->key().ToString();
		std::string value = it->value().ToString();
		Transaction tx{ value };
		transactionsPool.emplace(key, tx);
		for (const auto& i : tx.inputs)
			mempoolInputs[i.getUTXOKey()] = i;
	}
	Logger::log("POOL LOADED");
}

bool Blockchain::verifyDifficulty(const Hash& hash) {
	if (target == Hash{}) buildTarget();
	return hash <= target;
}

bool Blockchain::verifyInputs(const SignedTransaction& st) const {
	if (st.transaction.inputs.empty() || st.transaction.outputs.empty()) {
		return false;
	}
    Addr address = computeAddress(st.publicKey);
    uint64_t total_inputs = 0;
    uint64_t total_output = 0;
	std::unordered_set<std::string> seenInputs;
	for (const auto& in : st.transaction.inputs) {
		const std::string utxoKey = in.getUTXOKey();
		if (!seenInputs.emplace(utxoKey).second) return false;
		auto it = utxo.find(utxoKey);
		if (it == utxo.end()) return false;
		if (it->second.owner != address) return false;
		if (std::numeric_limits<uint64_t>::max() - total_inputs < it->second.coins) return false;
		total_inputs += it->second.coins;
	}
	for (const auto& out : st.transaction.outputs) {
		if (std::numeric_limits<uint64_t>::max() - total_output < out.coins) return false;
		total_output += out.coins;
	}
	return total_inputs >= total_output;
}

bool Blockchain::verifySignature(const SignedTransaction& st) const {
	return crypto_sign_verify_detached(st.signature.data(), st.transaction.transaction_hash.data(), st.transaction.transaction_hash.size(), st.publicKey.data()) == 0;
}

void Blockchain::updateUTXO(const Transaction& transaction) {
	for (const auto& in : transaction.inputs)
		utxo.erase(in.getUTXOKey());
	for (size_t i = 0; i < transaction.outputs.size(); i++) {
		const auto& output = transaction.outputs[i];
		std::string utxoKey = toHex(transaction.transaction_hash) + ":" + std::to_string(i);
		utxo.emplace(std::move(utxoKey), transaction.outputs[i]);
	}
}

std::expected<void, Blockchain::TransactionRejection> Blockchain::acceptTransaction(const SignedTransaction& st) {
	const auto& tx = st.transaction;
	if (tx.coins == 0) {
		return std::unexpected(TransactionRejection{
			TransactionErrorCode::InvalidAmount, "Invalid transaction amount"
		});
	}
	if (!verifyInputs(st)) {
		return std::unexpected(TransactionRejection{
			TransactionErrorCode::OwnershipVerificationFailed, "Ownership verification failed"
		});
	}
	if (!verifySignature(st)) {
		return std::unexpected(TransactionRejection{
			TransactionErrorCode::SignatureVerificationFailed, "Signature verification failed"
		});
	}
	const std::string transactionHashHex = toHex(tx.transaction_hash);
	if (transactionInPool(transactionHashHex)) {
		return std::unexpected(TransactionRejection{
			TransactionErrorCode::AlreadyInMempool, "Transaction is already in the mempool"
		});
	}
	for (const Input& input : tx.inputs) {
		if (mempoolInputs.contains(input.getUTXOKey())) {
			return std::unexpected(TransactionRejection{
				TransactionErrorCode::InputReservedByMempool,
				"Transaction input is already reserved by the mempool"
			});
		}
	}
	transactionsPool.emplace(transactionHashHex, tx);
	for (const Input& input : tx.inputs) {
		mempoolInputs.emplace(input.getUTXOKey(), input);
	}
	poolsDB.saveKey(transactionHashHex, tx.serializeTransaction());
	return {};
}

std::expected<void, std::string> Blockchain::addTransaction(const SignedTransaction& signedTransaction) {
	const auto result = acceptTransaction(signedTransaction);
	if (!result) {
		return std::unexpected(std::string{result.error().reason});
	}
	return {};
}

Hash Blockchain::getCurrentBlockHash() const {
	return blocks.back().blockHeader.hash;
}

bool Blockchain::transactionInPool(const std::string& txHash) const {
	return transactionsPool.find(txHash) != transactionsPool.end();
}

std::string Blockchain::serializeBlock(const Block& block) {
	return block.serialize();
}

Block Blockchain::deserializeBlock(const std::string& bytes) {
	return Block{ bytes };
}

bool Blockchain::verifyCoinbaseTransaction(const Transaction& tx) const {
    if(!tx.inputs.empty()) return false;
	if(tx.outputs.size() != 1) return false;
    return tx.outputs.front().coins <= MINER_REWARD;
}

bool Blockchain::verifyBlock(const Block& block){
    // No need to verify the transaction again.
    // Just check whether it exists in the transaction pool,
    // since the pool only contains verified transactions.
    if (block.transactions.empty()) return false;
    const Transaction& coinBaseTx = block.transactions.front();
    if(!verifyCoinbaseTransaction(coinBaseTx)) return false;
    std::vector<Hash> hashes;
    hashes.push_back(coinBaseTx.transaction_hash);
    for(std::size_t i = 1; i < block.transactions.size(); i++) {
        if(transactionsPool.find(toHex(block.transactions[i].transaction_hash)) == transactionsPool.end())
            return false;
        hashes.push_back(block.transactions[i].transaction_hash);
    }
    const Hash merkleRoot = Cryptography::computeMerkleRoot(hashes);
    if(merkleRoot != block.blockHeader.merkleRoot) return false;
    if(block.blockHeader.previous_hash != blocks.back().blockHeader.hash) return false;
    if(!verifyDifficulty(block.blockHeader.hash)) return false;
    return true;
}

Addr Blockchain::decodeAddressRequest(std::span<const unsigned char> payload) {
	if (payload.size() != AddrSize) {
		throw std::runtime_error("Address request payload has an invalid address size");
	}

	Addr address{};
	std::memcpy(address.data(), payload.data(), address.size());
	return address;
}

SignedTransaction Blockchain::deserializeCreateTransactionRequest(std::span<const unsigned char> payload) {
	const std::string_view rawPayload{
		reinterpret_cast<const char*>(payload.data()),
		payload.size()
	};
	BytesReader reader{ rawPayload };
	const PublicKey publicKey = reader.readBytes<PublicKey>();
	const Addr sender = reader.readBytes<AddrSize>();
	const Addr receiver = reader.readBytes<AddrSize>();
	const uint64_t amount = reader.readBytes<uint64_t>();
	const uint64_t timestamp = reader.readBytes<uint64_t>();

	const uint32_t inputCount = reader.readBytes<uint32_t>();
	constexpr size_t inputSize = HashSize + sizeof(uint32_t);
	constexpr size_t minimumTrailingBytes = sizeof(uint32_t) + sizeof(Signature);
	if (reader.remaining() < minimumTrailingBytes ||
		inputCount > (reader.remaining() - minimumTrailingBytes) / inputSize) {
		throw std::runtime_error("CreateTransaction request has an invalid input count");
	}
	std::vector<Input> inputs;
	inputs.reserve(inputCount);
	for (uint32_t index = 0; index < inputCount; ++index) {
		inputs.emplace_back(Input::deserialize(reader));
	}

	const uint32_t outputCount = reader.readBytes<uint32_t>();
	constexpr size_t outputSize = AddrSize + sizeof(uint64_t);
	if (reader.remaining() < sizeof(Signature) ||
		outputCount > (reader.remaining() - sizeof(Signature)) / outputSize) {
		throw std::runtime_error("CreateTransaction request has an invalid output count");
	}
	std::vector<UTXO> outputs;
	outputs.reserve(outputCount);
	for (uint32_t index = 0; index < outputCount; ++index) {
		outputs.emplace_back(UTXO::deserialize(reader));
	}

	const Signature signature = reader.readBytes<Signature>();
	if (reader.remaining() != 0) {
		throw std::runtime_error("CreateTransaction request contains trailing bytes");
	}

	return {
		Transaction{ sender, receiver, amount, std::move(inputs), std::move(outputs), timestamp },
		publicKey,
		signature
	};
}

Blockchain::AddressUtxos Blockchain::findAddressUtxos(const Addr& address) const {
	AddressUtxos addressUtxos;
	for (const auto& [utxoKey, output] : utxo) {
		if (output.owner != address) {
			continue;
		}
		if (std::numeric_limits<uint64_t>::max() - addressUtxos.totalCoins < output.coins) {
			throw std::runtime_error("UTXO balance exceeds the supported range");
		}
		addressUtxos.inputs.emplace_back(utxoKey);
		addressUtxos.totalCoins += output.coins;
	}
	return addressUtxos;
}

std::vector<unsigned char> Blockchain::serializeUtxosResponse(const AddressUtxos& addressUtxos) {
	if (addressUtxos.inputs.size() > std::numeric_limits<uint32_t>::max()) {
		throw std::runtime_error("UTXO response contains too many entries");
	}

	BytesWriter writer;
	writer.writeValues(static_cast<uint32_t>(addressUtxos.inputs.size()));
	for (const Input& input : addressUtxos.inputs) {
		writer.writeBytes(input.transaction_hash);
		writer.writeValues(input.output_index);
	}
	writer.writeValues(addressUtxos.totalCoins);
	return writer.getBuffer();
}

std::vector<unsigned char> Blockchain::serializeTransactionResponse(
	bool accepted,
	TransactionErrorCode errorCode,
	std::string_view reason
) {
	if (reason.size() > std::numeric_limits<uint16_t>::max()) {
		throw std::runtime_error("Transaction response reason is too long");
	}

	BytesWriter writer;
	writer.writeValues(static_cast<uint8_t>(accepted));
	writer.writeValues(static_cast<uint8_t>(errorCode));
	writer.writeValues(static_cast<uint16_t>(reason.size()));
	writer.writeBytes(reason);
	return writer.getBuffer();
}

asio::awaitable<void> Blockchain::sendPacket(
	PayloadType responseType,
	std::vector<unsigned char> payload,
	const std::shared_ptr<asio::ip::tcp::socket>& socket
) {
	const Packet packet{ responseType, payload };
	const std::vector<unsigned char> bytes = packet.getPacket();
	co_await asio::async_write(*socket, asio::buffer(bytes), asio::use_awaitable);
}

asio::awaitable<void> Blockchain::sendTransactionResponse(
	bool accepted,
	TransactionErrorCode errorCode,
	std::string_view reason,
	const std::shared_ptr<asio::ip::tcp::socket>& socket
) {
	co_await sendPacket(
		PayloadType::TransactionResponse,
		serializeTransactionResponse(accepted, errorCode, reason),
		socket
	);
}

asio::awaitable<void> Blockchain::readMessage(std::shared_ptr<asio::ip::tcp::socket> socket) {
    try {
		uint32_t payloadSize{};
		co_await asio::async_read(
			*socket,
			asio::buffer(&payloadSize, sizeof(payloadSize)),
			asio::use_awaitable
		);
		if (payloadSize < sizeof(PayloadType)) {
			throw std::runtime_error("Packet payload is smaller than its type field");
		}
		std::vector<unsigned char> buffer(payloadSize);
		co_await asio::async_read(*socket, asio::buffer(buffer), asio::use_awaitable);

		PayloadType payloadType;
		std::memcpy(&payloadType, buffer.data(), sizeof(payloadType));
		co_await handlePayload(payloadType, std::span{buffer}.subspan(sizeof(payloadType)), socket);
	} catch (const std::exception& exception) {
		Logger::error("Client message rejected: " + std::string{exception.what()});
	}
    co_return;
}

void Blockchain::acceptClient(){
    auto socket = std::make_shared<asio::ip::tcp::socket>(context);
    acceptor.async_accept(*socket, [socket, this](asio::error_code ec) {
        if (ec) {
            std::cerr << "Accept failed: " << ec.message() << '\n';
            acceptClient();
            return;
        }
        std::cout << "Client Connected!\n";
        acceptClient();
        asio::co_spawn(
            context,
            readMessage(socket),
            asio::detached
        );
    });
}

void Blockchain::setupConnection() {
    acceptClient();
    context.run();
}

asio::awaitable<void> Blockchain::handlePayload(PayloadType type, std::span<const unsigned char> payload, std::shared_ptr<asio::ip::tcp::socket> socket) {
    switch (type) {
		case PayloadType::GetUTXOs : {
            co_await handleGetUTXOs(payload, socket);
            break;
        }
		case PayloadType::GetUTXO:
            // co_await handleGetUTXO(payload, socket);
            break;
		case PayloadType::CreateTransaction:
			co_await handleCreateTransaction(payload, socket);
			break;
        default:
			Logger::error("Unsupported packet payload type");
    }
}
asio::awaitable<void> Blockchain::handleGetUTXOs(std::span<const unsigned char> payload, std::shared_ptr<asio::ip::tcp::socket> socket) {
	const Addr address = decodeAddressRequest(payload);
	const AddressUtxos addressUtxos = findAddressUtxos(address);
	co_await sendPacket(
		PayloadType::UTXOsResponse,
		serializeUtxosResponse(addressUtxos),
		socket
	);
}

asio::awaitable<void> Blockchain::handleCreateTransaction(
	std::span<const unsigned char> payload,
	std::shared_ptr<asio::ip::tcp::socket> socket
) {
	const auto parsedTransaction = [&]() -> std::expected<SignedTransaction, std::string> {
		try {
			return deserializeCreateTransactionRequest(payload);
		} catch (const std::exception& exception) {
			return std::unexpected(std::string{exception.what()});
		}
	}();
	if (!parsedTransaction) {
		Logger::reject("CreateTransaction payload rejected: " + parsedTransaction.error());
		co_await sendTransactionResponse(
			false, TransactionErrorCode::InvalidPayload, parsedTransaction.error(), socket
		);
		co_return;
	}

	const SignedTransaction& signedTransaction = *parsedTransaction;
	if (computeAddress(signedTransaction.publicKey) != signedTransaction.transaction.sender) {
		constexpr std::string_view reason = "Sender does not match the supplied public key";
		Logger::reject(std::string{reason});
		co_await sendTransactionResponse(
			false, TransactionErrorCode::SenderPublicKeyMismatch, reason, socket
		);
		co_return;
	}

	const auto acceptance = [&]() -> std::expected<void, TransactionRejection> {
		try {
			return acceptTransaction(signedTransaction);
		} catch (const std::exception& exception) {
			Logger::error("CreateTransaction could not be persisted: " + std::string{exception.what()});
			return std::unexpected(TransactionRejection{
				TransactionErrorCode::InternalError, "Transaction could not be persisted"
			});
		}
	}();
	if (!acceptance) {
		const TransactionRejection rejection = acceptance.error();
		Logger::reject("CreateTransaction rejected: " + std::string{rejection.reason});
		co_await sendTransactionResponse(false, rejection.code, rejection.reason, socket);
		co_return;
	}

	constexpr std::string_view reason = "Transaction accepted";
	Logger::log(std::string{reason});
	co_await sendTransactionResponse(true, TransactionErrorCode::None, reason, socket);
}
