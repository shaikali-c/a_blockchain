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
    std::string combined;
    combined.reserve(_publicKey.size() + _secretKey.size());
    combined.append(reinterpret_cast<const char*>(_publicKey.data()), _publicKey.size());
    combined.append(reinterpret_cast<const char*>(_secretKey.data()), _secretKey.size());
    keysDB.saveKey(kname, combined);
    std::cout << "Keys saved: \n\t" << toHex(_publicKey.data(), _publicKey.size()) << "\n";
}

void Keys::loadKeys(const std::string& kname) {
    std::string serialized = keysDB.loadKey(kname);
    const char* ptr = serialized.data();
    std::copy(ptr, ptr + _publicKey.size(), _publicKey.begin());
    ptr += _publicKey.size();
    std::copy(ptr, ptr + _secretKey.size(), _secretKey.begin());
    std::cout << "Keys loaded: \n\t" << toHex(_publicKey.data(), _publicKey.size()) << "\n";
}