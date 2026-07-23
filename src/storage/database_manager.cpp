#include "axis/storage/database_manager.h"

DatabaseManager::DatabaseManager(const std::string& path) {
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::DB* raw = nullptr;

    auto status = leveldb::DB::Open(options, path, &raw);

    if (!status.ok() || raw == nullptr) {
        throw std::runtime_error("DB open failed: " + status.ToString());
    }

    db.reset(raw);
}

std::string DatabaseManager::loadKey(const std::string& key) const {
    std::string value;
	const leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &value);
	if (!status.ok() && !status.IsNotFound()) {
		throw std::runtime_error("DB read failed for key '" + key + "': " + status.ToString());
	}
    return value;
}

void DatabaseManager::remove(const std::string& kname) const {
	const leveldb::Status status = db->Delete(leveldb::WriteOptions(), kname);
	if (!status.ok()) {
		throw std::runtime_error("DB delete failed for key '" + kname + "': " + status.ToString());
	}
}


std::string DatabaseManager::saveKey(const std::string& key, const std::string& value) const {
	const leveldb::Status status = db->Put(leveldb::WriteOptions(), key, value);
	if (!status.ok()) {
		throw std::runtime_error("DB write failed for key '" + key + "': " + status.ToString());
	}
    return value;
}
