#include "axis/net.h"
#include <cstring>

static std::string_view tx_error_str(TxError e) {
    switch (e) {
    case TxError::None:          return "accepted";
    case TxError::InvalidPayload: return "invalid payload";
    case TxError::BadPubkey:     return "bad public key";
    case TxError::ZeroAmount:    return "zero amount";
    case TxError::BadOwnership:  return "ownership failed";
    case TxError::BadSignature:  return "bad signature";
    case TxError::Duplicate:     return "duplicate";
    case TxError::InputSpent:    return "input spent";
    case TxError::Internal:      return "internal error";
    }
    return "unknown";
}

Server::Server(Chain& chain, uint16_t port)
    : acceptor_(ctx_, {asio::ip::tcp::v4(), port}), chain_(chain) {}

void Server::run() {
    do_accept();
    ctx_.run();
}

void Server::do_accept() {
    auto sock = std::make_shared<asio::ip::tcp::socket>(ctx_);
    acceptor_.async_accept(*sock, [this, sock](asio::error_code ec) {
        if (ec) {
            logging::err("accept: " + ec.message());
            do_accept();
            return;
        }
        logging::info("client connected");
        do_accept();
        asio::co_spawn(ctx_, handle_client(sock), asio::detached);
    });
}

asio::awaitable<void> Server::handle_client(
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    try {
        uint32_t payload_size{};
        co_await asio::async_read(*sock,
            asio::buffer(&payload_size, sizeof(payload_size)),
            asio::use_awaitable);

        if (payload_size < sizeof(MsgType))
            throw std::runtime_error("packet too small");

        std::vector<uint8_t> buf(payload_size);
        co_await asio::async_read(*sock, asio::buffer(buf),
            asio::use_awaitable);

        MsgType type;
        std::memcpy(&type, buf.data(), sizeof(type));
        co_await handle_msg(type,
            std::span{buf}.subspan(sizeof(type)), sock);
    } catch (const std::exception& e) {
        logging::err("client: " + std::string{e.what()});
    }
}

asio::awaitable<void> Server::handle_msg(
    MsgType type, std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    switch (type) {
    case MsgType::GetUTXOs:
        co_await on_get_utxos(payload, sock);
        break;
    case MsgType::CreateTransaction:
        co_await on_create_tx(payload, sock);
        break;
    default:
        logging::err("unknown msg type");
    }
}

asio::awaitable<void> Server::on_get_utxos(
    std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    Address addr{};
    if (payload.size() >= addr.size())
        std::memcpy(addr.data(), payload.data(), addr.size());

    std::vector<OutPoint> outpoints;
    uint64_t total = 0;
    chain_.get_utxos(addr, outpoints, total);

    auto response = serialize_utxo_response(outpoints, total);
    co_await send(sock, MsgType::UTXOsResponse, response);
}

asio::awaitable<void> Server::on_create_tx(
    std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    TxError err = TxError::InvalidPayload;
    std::string err_reason;

    try {
        Reader r{{reinterpret_cast<const char*>(payload.data()),
                  payload.size()}};

        auto pubkey = r.take_pk();
        auto timestamp = r.take_u64();

        uint32_t in_count = r.take_u32();
        std::vector<OutPoint> inputs;
        inputs.reserve(in_count);
        for (uint32_t i = 0; i < in_count; i++)
            inputs.push_back({r.take_hash(), r.take_u32()});

        uint32_t out_count = r.take_u32();
        std::vector<TxOutput> outputs;
        outputs.reserve(out_count);
        for (uint32_t i = 0; i < out_count; i++)
            outputs.push_back({r.take_addr(), r.take_u64()});

        auto sig = r.take_sig();

        Transaction tx{std::move(inputs), std::move(outputs), timestamp};
        SignedTransaction st{std::move(tx), pubkey, sig};

        err = chain_.add_tx(st);
        err_reason = std::string{tx_error_str(err)};

        if (err != TxError::None)
            logging::reject(err_reason);
        else
            logging::info("tx accepted");
    } catch (const std::exception& e) {
        err = TxError::InvalidPayload;
        err_reason = e.what();
        logging::reject("parse: " + err_reason);
    }

    auto response = serialize_tx_response(err, err_reason);
    co_await send(sock, MsgType::TransactionResponse, response);
}

asio::awaitable<void> Server::send(
    std::shared_ptr<asio::ip::tcp::socket> sock,
    MsgType type, std::span<const uint8_t> payload) {
    uint32_t total_size = sizeof(MsgType) + payload.size();
    std::vector<uint8_t> buf(sizeof(uint32_t) + total_size);
    std::memcpy(buf.data(), &total_size, sizeof(uint32_t));
    std::memcpy(buf.data() + sizeof(uint32_t), &type, sizeof(MsgType));
    std::memcpy(buf.data() + sizeof(uint32_t) + sizeof(MsgType),
                payload.data(), payload.size());

    co_await asio::async_write(*sock, asio::buffer(buf), asio::use_awaitable);
}

std::vector<uint8_t> Server::serialize_utxo_response(
    const std::vector<OutPoint>& outpoints, uint64_t total) {
    Writer w;
    w.put_u32(static_cast<uint32_t>(outpoints.size()));
    for (const auto& op : outpoints) {
        w.put_hash(op.txid);
        w.put_u32(op.index);
    }
    w.put_u64(total);
    return std::move(w.buf);
}

std::vector<uint8_t> Server::serialize_tx_response(
    TxError err, std::string_view reason) {
    Writer w;
    w.put_u8(static_cast<uint8_t>(err == TxError::None ? 1 : 0));
    w.put_u8(static_cast<uint8_t>(err));
    w.put_u16(static_cast<uint16_t>(reason.size()));
    w.put_str(reason);
    return std::move(w.buf);
}
