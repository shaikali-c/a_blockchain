#pragma once
#include <pch.h>
#include <sodium.h>
#include <databaseManager.h>
#include <common.h>

class Keys {
private:
	std::array<unsigned char, crypto_box_PUBLICKEYBYTES> _publicKey;
	std::array<unsigned char, crypto_box_SECRETKEYBYTES> _secretKey;
	static constexpr size_t ADDR_SIZE = crypto_generichash_BYTES;

public:
	Keys(const std::string& kName);
	Keys();

	void createKeys();
	void printKeys() const;
	void setOwner(const std::string&);

	std::string owner;
	std::string serializeKeys() const;
	void deserializeKeys(const std::string&);

	const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& publicKey() const;
	std::array<unsigned char, ADDR_SIZE> addr;
};