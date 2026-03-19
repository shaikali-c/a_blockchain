#include <databaseManager.h>

DBManager::DBManager(const std::string& path) {
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::DB* raw = nullptr;

    auto status = leveldb::DB::Open(options, path, &raw);

    if (!status.ok() || raw == nullptr) {
        throw std::runtime_error("DB open failed: " + status.ToString());
    }

    db.reset(raw);
}

std::string DBManager::loadKey(const std::string& key) const {
    std::string value;
    db->Get(leveldb::ReadOptions(), key, &value);
    return value;
}

void DBManager::remove(const std::string& kname) {
    db->Delete(leveldb::WriteOptions(), kname);
}


std::string DBManager::saveKey(const std::string& key, const std::string& value) const {
    db->Put(leveldb::WriteOptions(), key, value);
    return value;
}