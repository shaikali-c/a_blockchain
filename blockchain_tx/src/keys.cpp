#include <keys.h>

Keys::Keys(DBManager& db, const std::string& kname) : keysDB(db), owner(kname) {
    loadKeys(kname);
}

Keys::Keys(DBManager& db) : keysDB(db) {
    createKeys();
}

void Keys::setOwner(const std::string& o) {
    owner = o;
}

const std::array<unsigned char, crypto_box_PUBLICKEYBYTES>& Keys::publicKey() const {
    return _publicKey;
}

void Keys::printKeys() const {
    printKey(_publicKey);
}

void Keys::createKeys() {
    crypto_box_keypair(_publicKey.data(), _secretKey.data());
}

void Keys::saveKeys(const std::string& kname) {
    owner = kname;
    std::cout << "Keys saved: " << kname << "\n";
    std::string combined;
    combined.append(reinterpret_cast<const char*>(_publicKey.data()), _publicKey.size());
    combined.append(reinterpret_cast<const char*>(_secretKey.data()), _secretKey.size());
    keysDB.saveKey(kname, combined);
}

void Keys::loadKeys(const std::string& kname) {
    std::cout << "Keys loaded: " << kname << "\n";
    std::string serialized = keysDB.loadKey(kname);
    std::memcpy(_publicKey.data(), serialized.data(), crypto_box_PUBLICKEYBYTES);
    std::memcpy(_secretKey.data(), serialized.data() + crypto_box_PUBLICKEYBYTES, crypto_box_SECRETKEYBYTES);
}