#pragma once
#include <pch.h>

class DBManager {
public:
	std::unique_ptr<leveldb::DB> db;
	DBManager(const std::string& path);
	std::string loadKey(const std::string& key) const;
	std::string saveKey(const std::string& key, const std::string& value) const;
	void remove(const std::string&) const;
};