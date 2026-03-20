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

    const size_t pubSize = _publicKey.size();
    const size_t secSize = _secretKey.size();

    std::string combined(pubSize + secSize, '\0');

    std::memcpy(combined.data(), _publicKey.data(), pubSize);
    std::memcpy(combined.data() + pubSize, _secretKey.data(), secSize);

    keysDB.saveKey(kname, combined);

}

void Keys::loadKeys(const std::string& kname) {
    std::string serialized = keysDB.loadKey(kname);

    const size_t pubSize = _publicKey.size();
    const size_t secSize = _secretKey.size();

    if (serialized.size() != pubSize + secSize) {
        throw std::runtime_error("Invalid key data size");
    }

    std::memcpy(_publicKey.data(), serialized.data(), pubSize);
    std::memcpy(_secretKey.data(), serialized.data() + pubSize, secSize);

}