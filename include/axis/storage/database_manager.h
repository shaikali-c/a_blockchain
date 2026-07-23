#pragma once
#include <pch.h>

class DatabaseManager {
public:
	DatabaseManager(const std::string& path);

	std::unique_ptr<leveldb::DB> db;
	std::string loadKey(const std::string& key) const;
	std::string saveKey(const std::string& key, const std::string& value) const;

	void remove(const std::string&) const;
};
