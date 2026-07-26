#include "axis/net.h"

#include "axis/block.h"
#include "axis/tx.h"
#include "axis/util.h"

#include <cstdint>
#include <cstring>
#include <expected>
#include <utility>

namespace {

struct ParsedCreateTx {
    PublicKey pubkey;
    Timestamp timestamp;
    std::vector<OutPoint> inputs;
    std::vector<TxOutput> outputs;
    Signature sig;
};

struct ParsedCreateBlock {
    Hash prev_hash;
    Hash wire_merkle;
    Timestamp timestamp;
    uint64_t nonce;
    Transaction coinbase;
    std::vector<Transaction> transactions;
};

std::string_view tx_error_str(TxError e) {
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

std::vector<uint8_t> serialize_block_response(
    BlockError err,
    std::string_view reason
) {
    Writer w;
    w.put_u8(static_cast<uint8_t>(err == BlockError::None ? 1 : 0));
    w.put_u8(static_cast<uint8_t>(err));
    w.put_u16(static_cast<uint16_t>(reason.size()));
    w.put_str(reason);
    return std::move(w.buf);
}

ParsedCreateTx parse_create_tx_payload(std::span<const uint8_t> payload) {
    Reader r{{reinterpret_cast<const char*>(payload.data()), payload.size()}};

    ParsedCreateTx parsed{
        .pubkey = r.take_pk(),
        .timestamp = Timestamp{r.take_u64()}
    };

    const uint32_t in_count = r.take_u32();
    parsed.inputs.reserve(in_count);
    for (uint32_t i = 0; i < in_count; ++i) {
        parsed.inputs.push_back({r.take_hash(), r.take_u32()});
    }

    const uint32_t out_count = r.take_u32();
    parsed.outputs.reserve(out_count);
    for (uint32_t i = 0; i < out_count; ++i) {
        parsed.outputs.push_back({r.take_addr(), r.take_u64()});
    }

    parsed.sig = r.take_sig();
    return parsed;
}

std::expected<ParsedCreateBlock, std::string> parse_create_block_payload(
    std::span<const uint8_t> payload,
    const Chain& chain
) {
    Reader r{{reinterpret_cast<const char*>(payload.data()), payload.size()}};

    const Hash prev_hash = r.take_hash();
    const Hash wire_merkle = r.take_hash();
    const Timestamp timestamp{r.take_u64()};
    const uint64_t nonce = r.take_u64();

    const Address cb_address = r.take_addr();
    const uint64_t cb_reward = r.take_u64();
    const uint64_t cb_timestamp = r.take_u64();
    Transaction coinbase{{}, {{cb_address, cb_reward}}, Timestamp{cb_timestamp}};

    const uint32_t tx_count = r.take_u32();
    std::vector<Transaction> transactions;
    transactions.reserve(tx_count + 1);
    transactions.push_back(coinbase);

    for (uint32_t i = 0; i < tx_count; ++i) {
        const Hash txid = r.take_hash();
        if (!chain.pool_contains(txid)) {
            return std::unexpected("tx not in pool");
        }
        transactions.push_back(chain.get_pool_tx(txid));
    }

    return ParsedCreateBlock{
        .prev_hash = prev_hash,
        .wire_merkle = wire_merkle,
        .timestamp = timestamp,
        .nonce = nonce,
        .coinbase = std::move(coinbase),
        .transactions = std::move(transactions)
    };
}

} // namespace

Server::Server(Chain& chain, uint16_t port, ServerEvents events)
    : acceptor_(ctx_, {asio::ip::tcp::v4(), port}), chain_(chain),
      events_(std::move(events)) {}

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
        do_accept();
        asio::co_spawn(ctx_, handle_client(sock), asio::detached);
    });
}
asio::awaitable<void> Server::handle_client(
    std::shared_ptr<asio::ip::tcp::socket> sock)
{
    try {
        while (true) {
            uint32_t payload_size{};

            co_await asio::async_read(
                *sock,
                asio::buffer(&payload_size, sizeof(payload_size)),
                asio::use_awaitable);

            if (payload_size < sizeof(MsgType))
                throw std::runtime_error("packet too small");

            std::vector<uint8_t> buf(payload_size);

            co_await asio::async_read(
                *sock,
                asio::buffer(buf),
                asio::use_awaitable);

            MsgType type;
            std::memcpy(&type, buf.data(), sizeof(type));

            co_await handle_msg(
                type,
                std::span(buf).subspan(sizeof(type)),
                sock);
        }
    }
    catch (const std::exception& e) {
        const std::string msg = e.what();
        if (msg == "End of file") {
            co_return;
        }
        logging::err("client session ended: " + msg);
    }
}

asio::awaitable<void> Server::handle_msg(
    MsgType type, std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    switch (type) {
    case MsgType::GetUTXOs:
        co_await on_get_utxos(payload, sock);
        break;
    case MsgType::GetTip:
        co_await on_get_tip(payload, sock);
        break;
    case MsgType::GetPool:
        co_await on_get_pool(payload, sock);
        break;
    case MsgType::CreateTransaction:
        co_await on_create_tx(payload, sock);
        break;
    case MsgType::GetDifficulty:
        co_await on_get_difficulty(payload, sock);
        break;
    case MsgType::CreateBlock:
        co_await on_create_block(payload, sock);
        break;

    default:
        logging::err("unknown msg type");
    }
}

asio::awaitable<void> Server::on_get_difficulty(
    std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    co_await send(sock, MsgType::DifficultyResponse, std::vector<uint8_t>{chain_.get_difficulty()});
}

asio::awaitable<void> Server::on_get_pool(
    std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    uint32_t txCount = chain_.get_pool_txs().size();
    Writer poolWriter;
    poolWriter.put_u32(txCount);
    for (const auto& tx : chain_.get_pool_txs()) {
        poolWriter.put_hash(tx.txid());
    }
    co_await send(sock, MsgType::PoolResponse, poolWriter.buf);
}

asio::awaitable<void> Server::on_get_tip(
    std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    Hash tipHash = chain_.tip().hash();
    Writer w;
    w.put_hash(tipHash);
    co_await send(sock, MsgType::TipResponse, w.buf);
}

asio::awaitable<void> Server::on_get_utxos(
    std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    Address addr{};
    if (payload.size() >= addr.size())
        std::memcpy(addr.data(), payload.data(), addr.size());

    std::vector<std::pair<OutPoint, uint64_t>> outpoints;
    chain_.get_utxos(addr, outpoints);

    auto response = serialize_utxo_response(outpoints);
    co_await send(sock, MsgType::UTXOsResponse, response);
}

asio::awaitable<void> Server::on_create_tx(
    std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    TxError err = TxError::InvalidPayload;
    std::string err_reason;

    try {
        ParsedCreateTx parsed = parse_create_tx_payload(payload);

        Transaction tx{
            std::move(parsed.inputs),
            std::move(parsed.outputs),
            parsed.timestamp
        };
        SignedTransaction st{std::move(tx), parsed.pubkey, parsed.sig};

        err = chain_.add_tx(st);
        err_reason = std::string{tx_error_str(err)};

        if (err != TxError::None) {
            logging::reject(err_reason);
        } else {
            logging::info("tx accepted");
            if (events_.on_tx_accepted)
                events_.on_tx_accepted(st.tx);
        }

        std::cout << st.tx << "\n";
    } catch (const std::exception& e) {
        err = TxError::InvalidPayload;
        err_reason = e.what();
        logging::reject("parse: " + err_reason);
    }

    const auto response = serialize_tx_response(err, err_reason);
    co_await send(sock, MsgType::TransactionResponse, response);
}

asio::awaitable<void> Server::on_create_block(
    std::span<const uint8_t> payload,
    std::shared_ptr<asio::ip::tcp::socket> sock) {
    BlockError err = BlockError::InvalidPayload;
    std::string err_reason;

    try {
        auto parsed = parse_create_block_payload(payload, chain_);
        if (!parsed) {
            err_reason = parsed.error();
            logging::reject("block parse: " + err_reason);
        } else {
            Block blk{
                parsed->prev_hash,
                std::move(parsed->transactions),
                parsed->timestamp,
                parsed->nonce
            };

            if (blk.header().merkle_root != parsed->wire_merkle) {
                err = BlockError::InvalidBlockHash;
                err_reason = "merkle root mismatch";
            } else if (blk.hash() > chain_.target()) {
                err = BlockError::HighHash;
                err_reason = "hash does not meet difficulty";
                std::cout << "hash: " << to_hex(blk.hash())
                          << " target: " << to_hex(chain_.target()) << "\n";
            } else if (blk.header().prev_hash != chain_.tip_hash()) {
                err = BlockError::BadPreviousHash;
                err_reason = "prev hash does not match tip";
            } else {
                chain_.add_block(blk);
                err = BlockError::None;
                err_reason = "accepted";
                logging::info(
                    "block accepted at height " + std::to_string(chain_.height())
                );
                if (events_.on_block_accepted)
                    events_.on_block_accepted(blk);
                std::cout << blk << "\n";
            }
        }
    } catch (const std::exception& e) {
        err = BlockError::InvalidPayload;
        err_reason = e.what();
        logging::reject("block parse: " + err_reason);
    }

    const auto response = serialize_block_response(err, err_reason);
    co_await send(sock, MsgType::CreateBlockResponse, response);
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
    const std::vector<std::pair<OutPoint, uint64_t>>& outpoints) {
    Writer w;
    w.put_u32(static_cast<uint32_t>(outpoints.size()));
    for (const auto& [op, amount] : outpoints) {
        w.put_hash(op.txid);
        w.put_u32(op.index);
        w.put_u64(amount);
    }
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
