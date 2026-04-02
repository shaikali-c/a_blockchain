#include <keys.h>

Keys::Keys(const std::string& kName): owner(kName), addr(_hashBytes(_publicKey)) {
    deserializeKeys(kName);
    addr = _hashBytes(_publicKey);
    _pHex.resize(2 * _publicKey.size() + 1);
    sodium_bin2hex(_pHex.data(), _pHex.size(), _publicKey.data(), _publicKey.size());
}

Keys::Keys(): addr(_hashBytes(_publicKey)) {
    createKeys();
    addr = _hashBytes(_publicKey);
    _pHex.resize(2 * _publicKey.size() + 1);
    sodium_bin2hex(_pHex.data(), _pHex.size(), _publicKey.data(), _publicKey.size());
}

void Keys::setOwner(const std::string& o) {
    owner = o;
}

const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>& Keys::publicKey() const {
    return _publicKey;
}

void Keys::createKeys() {
    crypto_sign_keypair(_publicKey.data(), _secretKey.data());
}

std::array<unsigned char, crypto_sign_BYTES> Keys::sign(
    const unsigned char* data, size_t len)
{
    std::array<unsigned char, crypto_sign_BYTES> signature;
    unsigned long long signature_len = 0;

    if (crypto_sign_detached(
        signature.data(),
        NULL,
        data,
        len,
        _secretKey.data()) != 0)
    {
        throw std::runtime_error("Signing failed");
    }

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

std::pair<Transaction, std::array<unsigned char, crypto_sign_BYTES>> Keys::createTransaction(const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>&receiver, uint64_t amount, std::vector<Input> inputs, uint64_t total) {
    std::vector<UTXO> outputs;
    outputs.emplace_back(amount, receiver);

    if (total > amount) {
        UTXO change{ total - amount, addr };
        outputs.push_back(change);
    }

    Transaction transaction{ _publicKey, receiver, amount, inputs, outputs };
    std::array<unsigned char, crypto_sign_BYTES> signature = sign(transaction.transaction_hash_bytes.data(), transaction.transaction_hash_bytes.size());

    return std::make_pair(transaction, signature);
}