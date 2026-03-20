#pragma once
#include <pch.h>
#include <sodium.h>
#include <databaseManager.h>
#include <common.h>

class Keys {
private:
	std::array<unsigned char, crypto_box_PUBLICKEYBYTES> _publicKey;
	std::array<unsigned char, crypto_box_SECRETKEYBYTES> _secretKey;
	DBManager& keysDB;

public:
	std::string owner;
	Keys(DBManager& db, const std::string& kname);
	Keys(DBManager& db);
	void createKeys();
	void saveKeys(const std::string& kname);
	void printKeys() const;
	void setOwner(const std::string& o);
	void loadKeys(const std::string& kname);
	const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& publicKey() const;
};