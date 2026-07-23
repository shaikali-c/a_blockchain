#pragma once
#include "pch.h"
#include <sodium.h>

using SecretKey = std::array<unsigned char, crypto_sign_SECRETKEYBYTES>;
using PublicKey = std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>;
using Hash = std::array<unsigned char, crypto_generichash_BYTES>;
using Signature = std::array<unsigned char, crypto_sign_BYTES>;
using Addr = std::array<unsigned char, 20>;

constexpr size_t HashSize = crypto_generichash_BYTES;
constexpr size_t AddrSize = 20;

enum class PayloadType : uint16_t {
  GetBalance,
  GetBlock,
  GetTransaction,
  GetUTXO,
  GetUTXOs,
  BalanceResponse,
  BlockResponse,
  TransactionResponse,
  UTXOResponse,
  UTXOsResponse,
  CreateTransaction
};

// Kept for source compatibility; packet values and layout are unchanged.
using PayLoadType = PayloadType;

enum class TransactionErrorCode : uint8_t {
	None = 0,
	InvalidPayload,
	SenderPublicKeyMismatch,
	InvalidAmount,
	OwnershipVerificationFailed,
	SignatureVerificationFailed,
	AlreadyInMempool,
	InputReservedByMempool,
	InternalError
};

struct Packet {
  PayloadType payloadType;
  std::vector<unsigned char> payload;
  Packet(PayloadType pt, const std::vector<unsigned char>& p) : payloadType(pt), payload(p){}
  Packet(const std::vector<unsigned char>& p){
	  if (p.size() < sizeof(payloadType)) {
		  throw std::runtime_error("Packet is smaller than its payload type");
	  }
      std::memcpy(&payloadType, p.data(), sizeof(payloadType));
      payload.assign(
          p.begin() + sizeof(payloadType),
          p.end()
      );
  }
  PayloadType getPayloadType() const {
      return payloadType;
  }
  std::vector<unsigned char> getPacket() const {
      uint32_t size = sizeof(payloadType) + static_cast<uint32_t>(payload.size());
      std::vector<unsigned char> buffer(sizeof(size) + size);
      std::memcpy(buffer.data(), &size, sizeof(size));
      std::memcpy(buffer.data() + sizeof(size), &payloadType, sizeof(payloadType));
      std::memcpy(buffer.data() + sizeof(size) + sizeof(payloadType), payload.data(), payload.size());
      return buffer;
  }
};

struct BytesWriter {
	std::vector<unsigned char> buffer;

	template<typename T>
	void writeBytes(const T& bytes) {
		buffer.insert(buffer.end(), bytes.begin(), bytes.end());
	}

	template<typename T>
	void writeValues(T value) {
		static_assert(std::is_trivially_copyable_v<T>);
		auto* bytes = reinterpret_cast<unsigned char*>(&value);
		for (size_t i = 0; i < sizeof(T); i++) buffer.push_back(bytes[i]);
	}
	std::vector<unsigned char> getBuffer() const {
		return buffer;
	}
	std::string getStringBytes() const {
		return std::string(buffer.begin(), buffer.end());
	}
};

struct BytesReader {
	std::string_view rawBytes;
	size_t offset = 0;
	explicit BytesReader(std::string_view bytes) : rawBytes(bytes) {}

	[[nodiscard]] size_t remaining() const noexcept {
		return rawBytes.size() - offset;
	}

	void require(size_t byteCount) const {
		if (byteCount > remaining()) {
			throw std::runtime_error("Unexpected end of serialized data");
		}
	}

	template <typename T>
	T readBytes() {
		static_assert(std::is_trivially_copyable_v<T>);
		require(sizeof(T));
		T value{};
		std::memcpy(&value, rawBytes.data() + offset, sizeof(T));
		offset += sizeof(T);
		return value;
	}
	template <size_t S>
	std::array<unsigned char, S> readBytes() {
		require(S);
		std::array<unsigned char, S> value{};
		std::memcpy(value.data(), rawBytes.data() + offset, value.size());
		offset += value.size();
		return value;
	}
	std::string readBytesString(size_t size) {
		require(size);
		std::string result(rawBytes.data() + offset, size);
		offset += size;
		return result;
	}
};

template <std::size_t N>
std::string toHex(const std::array<unsigned char, N>& arr) {
	std::string hex;
	hex.resize(N * 2 + 1);
	sodium_bin2hex(
		hex.data(),
		hex.size(),
		arr.data(),
		arr.size()
	);
	hex.pop_back();
	return hex;
}
template <std::size_t N>
std::array<unsigned char, N> toBytes(const std::string& hex) {
	if (hex.size() != N * 2) {
		throw std::runtime_error("Invalid hex length");
	}
	std::array<unsigned char, N> bytes;
	size_t bin_len;
	if (sodium_hex2bin(
		bytes.data(),
		bytes.size(),
		hex.data(),
		hex.size(),
		nullptr,
		&bin_len,
		nullptr
	) != 0 || bin_len != bytes.size()) {
		throw std::runtime_error("Invalid hexadecimal data");
	}
	return bytes;
}
template <std::size_t N>
Hash hashBytes(const std::array<unsigned char, N>& bytes) {
	Hash hash{};
	crypto_generichash(hash.data(), hash.size(), bytes.data(), bytes.size(), nullptr, 0);
	return hash;
}
Hash hashBytesVector(const std::vector<unsigned char>& bytes);
Addr computeAddress(const PublicKey& pk);
void appendBytes(std::string& buffer, const void* data, size_t size);

struct UTXOKey {
    Hash txHash;
    uint32_t index;
};

UTXOKey parseUTXOKey(const std::string& key);
