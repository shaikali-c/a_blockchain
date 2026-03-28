#include <keys.h>

Keys::Keys(const std::string& kName): owner(kName), addr(_hashBytes(_publicKey)) {
    deserializeKeys(kName);
    addr = _hashBytes(_publicKey);
}

Keys::Keys(): addr(_hashBytes(_publicKey)) {
    createKeys();
}

void Keys::setOwner(const std::string& o) {
    owner = o;
}

const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& Keys::publicKey() const {
    return _publicKey;
}

void Keys::printKeys() const {
    printKey(_publicKey);
}

void Keys::createKeys() {
    crypto_sign_keypair(_publicKey.data(), _secretKey.data());
}

std::vector<unsigned char> Keys::sign(const std::string& data) {
    std::vector<unsigned char> signature(crypto_sign_BYTES);
    unsigned long long signature_len = 0;

    int result = crypto_sign_detached(
        signature.data(),
        &signature_len,
        reinterpret_cast<const unsigned char*>(data.data()),
        data.size(),
        _secretKey.data()
    );

    if (result != 0) {
        throw std::runtime_error("Signing failed"); 
    }

    signature.resize(signature_len);
    return signature;
}

std::string Keys::serializeKeys() const {

    const size_t pubSize = _publicKey.size();
    const size_t secSize = _secretKey.size();

    std::string combined(pubSize + secSize, '\0');

    std::memcpy(combined.data(), _publicKey.data(), pubSize);
    std::memcpy(combined.data() + pubSize, _secretKey.data(), secSize);

    return combined;

}

void Keys::deserializeKeys(const std::string& serialized) {
    const size_t pubSize = _publicKey.size();
    const size_t secSize = _secretKey.size();

    if (serialized.size() != pubSize + secSize) throw std::runtime_error("Invalid key data size");

    std::memcpy(_publicKey.data(), serialized.data(), pubSize);
    std::memcpy(_secretKey.data(), serialized.data() + pubSize, secSize);

}