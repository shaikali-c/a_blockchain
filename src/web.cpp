#include "axis/web.h"

#include "axis/util.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <exception>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr uint32_t kDefaultBlockCount = 10;
constexpr uint32_t kMaxBlockCount = 100;
constexpr size_t kMaxRawTxHexChars = 128 * 1024;

std::string json_escape(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char ch : value) {
        switch (ch) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20) {
                out += "\\u00";
                const char* hex = "0123456789abcdef";
                out += hex[(ch >> 4) & 0x0f];
                out += hex[ch & 0x0f];
            } else {
                out += ch;
            }
        }
    }
    return out;
}

crow::response json_response(int code, std::string body) {
    crow::response res{code, std::move(body)};
    res.set_header("Content-Type", "application/json");
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, X-API-Key");
    return res;
}

crow::response error_response(int code, std::string_view message) {
    std::ostringstream os;
    os << "{\"error\":\"" << json_escape(message) << "\",\"code\":" << code << "}";
    return json_response(code, os.str());
}

bool parse_u32(std::string_view text, uint32_t& out) {
    if (text.empty())
        return false;
    uint64_t value = 0;
    const auto* first = text.data();
    const auto* last = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last || value > std::numeric_limits<uint32_t>::max())
        return false;
    out = static_cast<uint32_t>(value);
    return true;
}

std::vector<uint8_t> hex_to_bytes(std::string_view hex) {
    if (hex.size() % 2 != 0)
        throw std::runtime_error("hex input must contain an even number of characters");

    std::vector<uint8_t> bytes(hex.size() / 2);
    if (bytes.empty())
        return bytes;

    size_t bin_len = 0;
    if (sodium_hex2bin(bytes.data(), bytes.size(), hex.data(), hex.size(),
                       nullptr, &bin_len, nullptr) != 0 || bin_len != bytes.size()) {
        throw std::runtime_error("invalid hex data");
    }
    return bytes;
}

SignedTransaction parse_signed_transaction_payload(std::span<const uint8_t> payload) {
    Reader r{{reinterpret_cast<const char*>(payload.data()), payload.size()}};

    PublicKey pubkey = r.take_pk();
    Timestamp timestamp{r.take_u64()};

    const uint32_t in_count = r.take_u32();
    std::vector<OutPoint> inputs;
    inputs.reserve(in_count);
    for (uint32_t i = 0; i < in_count; ++i)
        inputs.push_back({r.take_hash(), r.take_u32()});

    const uint32_t out_count = r.take_u32();
    std::vector<TxOutput> outputs;
    outputs.reserve(out_count);
    for (uint32_t i = 0; i < out_count; ++i)
        outputs.push_back({r.take_addr(), r.take_u64()});

    Signature sig = r.take_sig();
    if (r.offset != r.data.size())
        throw std::runtime_error("trailing bytes after transaction payload");

    return SignedTransaction{
        Transaction{std::move(inputs), std::move(outputs), timestamp},
        pubkey,
        sig
    };
}

std::string tx_error_to_string(TxError error) {
    switch (error) {
    case TxError::None: return "accepted";
    case TxError::InvalidPayload: return "invalid payload";
    case TxError::BadPubkey: return "bad public key";
    case TxError::ZeroAmount: return "zero amount";
    case TxError::BadOwnership: return "ownership failed";
    case TxError::BadSignature: return "bad signature";
    case TxError::Duplicate: return "duplicate";
    case TxError::InputSpent: return "input spent";
    case TxError::Internal: return "internal error";
    }
    return "unknown";
}

std::string outpoint_json(const OutPoint& outpoint) {
    std::ostringstream os;
    os << "{\"txid\":\"" << to_hex(outpoint.txid) << "\",\"index\":" << outpoint.index << "}";
    return os.str();
}

std::string output_json(const TxOutput& output) {
    std::ostringstream os;
    os << "{\"recipient\":\"" << to_hex(output.recipient) << "\",\"amount\":" << output.amount << "}";
    return os.str();
}

std::string transaction_json(const Transaction& tx) {
    std::ostringstream os;
    os << "{\"txid\":\"" << to_hex(tx.txid()) << "\","
       << "\"timestamp\":" << tx.timestamp.value << ","
       << "\"coinbase\":" << (tx.is_coinbase() ? "true" : "false") << ","
       << "\"inputs\":[";
    for (size_t i = 0; i < tx.inputs.size(); ++i) {
        if (i > 0)
            os << ',';
        os << outpoint_json(tx.inputs[i]);
    }
    os << "],\"outputs\":[";
    for (size_t i = 0; i < tx.outputs.size(); ++i) {
        if (i > 0)
            os << ',';
        os << output_json(tx.outputs[i]);
    }
    os << "]}";
    return os.str();
}

std::string block_summary_json(const Block& block, uint32_t height) {
    std::ostringstream os;
    os << "{\"height\":" << height << ","
       << "\"hash\":\"" << to_hex(block.hash()) << "\","
       << "\"previousHash\":\"" << to_hex(block.header().prev_hash) << "\","
       << "\"merkleRoot\":\"" << to_hex(block.header().merkle_root) << "\","
       << "\"timestamp\":" << block.header().timestamp.value << ","
       << "\"nonce\":" << block.header().nonce << ","
       << "\"transactionCount\":" << block.transactions.size() << "}";
    return os.str();
}

std::string block_json(const Block& block, uint32_t height) {
    std::ostringstream os;
    os << "{\"height\":" << height << ","
       << "\"hash\":\"" << to_hex(block.hash()) << "\","
       << "\"previousHash\":\"" << to_hex(block.header().prev_hash) << "\","
       << "\"merkleRoot\":\"" << to_hex(block.header().merkle_root) << "\","
       << "\"timestamp\":" << block.header().timestamp.value << ","
       << "\"nonce\":" << block.header().nonce << ","
       << "\"txids\":[";
    for (size_t i = 0; i < block.transactions.size(); ++i) {
        if (i > 0)
            os << ',';
        os << '"' << to_hex(block.transactions[i].txid()) << '"';
    }
    os << "],\"transactions\":[";
    for (size_t i = 0; i < block.transactions.size(); ++i) {
        if (i > 0)
            os << ',';
        os << transaction_json(block.transactions[i]);
    }
    os << "]}";
    return os.str();
}

std::string event_new_tx_json(const Transaction& tx) {
    std::ostringstream os;
    os << "{\"type\":\"new_tx\",\"txid\":\"" << to_hex(tx.txid())
       << "\",\"transaction\":" << transaction_json(tx) << "}";
    return os.str();
}

std::string event_new_block_json(const Block& block) {
    std::ostringstream os;
    os << "{\"type\":\"new_block\",\"hash\":\"" << to_hex(block.hash())
       << "\",\"timestamp\":" << block.header().timestamp.value
       << ",\"transactionCount\":" << block.transactions.size() << "}";
    return os.str();
}

} // namespace

WebServer::WebServer(Chain& chain, uint16_t port)
    : chain_(chain), port_(port) {
    setup_routes();
}

void WebServer::run() {
    app_.port(port_).multithreaded().run();
}

void WebServer::stop() {
    app_.stop();
}

void WebServer::broadcast_new_tx(const Transaction& tx) {
    broadcast_text(event_new_tx_json(tx));
}

void WebServer::broadcast_new_block(const Block& block) {
    broadcast_text(event_new_block_json(block));
}

void WebServer::broadcast_text(const std::string& message) {
    std::lock_guard lock(ws_mutex_);
    for (auto* connection : ws_connections_)
        connection->send_text(message);
}

void WebServer::setup_routes() {
    CROW_ROUTE(app_, "/api/status").methods(crow::HTTPMethod::GET)
    ([this]() {
        std::ostringstream os;
        os << "{\"status\":\"online\","
           << "\"version\":\"0.1\","
           << "\"blockHeight\":" << chain_.height() << ","
           << "\"difficulty\":" << static_cast<uint32_t>(chain_.get_difficulty()) << ","
           << "\"tipHash\":\"" << to_hex(chain_.tip_hash()) << "\"}";
        return json_response(200, os.str());
    });

    CROW_ROUTE(app_, "/api/tip").methods(crow::HTTPMethod::GET)
    ([this]() {
        const auto height = chain_.height();
        if (height == 0)
            return error_response(404, "chain is empty");
        return json_response(200, block_json(chain_.tip(), height - 1));
    });

    CROW_ROUTE(app_, "/api/block/<string>").methods(crow::HTTPMethod::GET)
    ([this](const std::string& id) {
        uint32_t height = 0;
        if (parse_u32(id, height)) {
            auto block = chain_.get_block(height);
            if (!block)
                return error_response(404, "block not found");
            return json_response(200, block_json(*block, height));
        }

        try {
            const auto hash = from_hex<32>(id);
            auto block = chain_.get_block(hash);
            if (!block)
                return error_response(404, "block not found");
            return json_response(200, block_json(block->second, block->first));
        } catch (const std::exception&) {
            return error_response(400, "block id must be a height or 32-byte hex hash");
        }
    });

    CROW_ROUTE(app_, "/api/blocks").methods(crow::HTTPMethod::GET)
    ([this](const crow::request& req) {
        uint32_t start = 0;
        uint32_t count = kDefaultBlockCount;
        if (const char* value = req.url_params.get("start"); value && !parse_u32(value, start))
            return error_response(400, "start must be an unsigned integer");
        if (const char* value = req.url_params.get("count"); value && !parse_u32(value, count))
            return error_response(400, "count must be an unsigned integer");
        count = std::min(count, kMaxBlockCount);

        const auto blocks = chain_.get_blocks(start, count);
        std::ostringstream os;
        os << "{\"blocks\":[";
        for (size_t i = 0; i < blocks.size(); ++i) {
            if (i > 0)
                os << ',';
            os << block_summary_json(blocks[i], start + static_cast<uint32_t>(i));
        }
        os << "],\"total\":" << chain_.height() << "}";
        return json_response(200, os.str());
    });

    CROW_ROUTE(app_, "/api/mempool").methods(crow::HTTPMethod::GET)
    ([this]() {
        const auto txs = chain_.get_pool_txs();
        std::ostringstream os;
        os << "{\"size\":" << txs.size() << ",\"txids\":[";
        for (size_t i = 0; i < txs.size(); ++i) {
            if (i > 0)
                os << ',';
            os << '"' << to_hex(txs[i].txid()) << '"';
        }
        os << "],\"transactions\":[";
        for (size_t i = 0; i < txs.size(); ++i) {
            if (i > 0)
                os << ',';
            os << transaction_json(txs[i]);
        }
        os << "]}";
        return json_response(200, os.str());
    });

    auto utxos_handler = [this](const std::string& address_hex) {
        Address address{};
        try {
            address = from_hex<20>(address_hex);
        } catch (const std::exception&) {
            return error_response(400, "address must be a 20-byte hex value");
        }

        std::vector<std::pair<OutPoint, uint64_t>> utxos;
        chain_.get_utxos(address, utxos);

        uint64_t balance = 0;
        std::ostringstream os;
        os << "{\"address\":\"" << to_hex(address) << "\",\"balance\":";
        for (const auto& [_, amount] : utxos)
            balance += amount;
        os << balance << ",\"utxos\":[";
        for (size_t i = 0; i < utxos.size(); ++i) {
            if (i > 0)
                os << ',';
            const auto& [outpoint, amount] = utxos[i];
            os << "{\"txid\":\"" << to_hex(outpoint.txid) << "\","
               << "\"index\":" << outpoint.index << ","
               << "\"amount\":" << amount << "}";
        }
        os << "]}";
        return json_response(200, os.str());
    };

    CROW_ROUTE(app_, "/api/utxos/<string>").methods(crow::HTTPMethod::GET)(utxos_handler);
    CROW_ROUTE(app_, "/api/address/<string>").methods(crow::HTTPMethod::GET)(utxos_handler);

    CROW_ROUTE(app_, "/api/transaction").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("rawTx") || body["rawTx"].t() != crow::json::type::String)
            return error_response(400, "request body must include string field rawTx");

        const std::string raw_tx = body["rawTx"].s();
        if (raw_tx.size() > kMaxRawTxHexChars)
            return error_response(413, "rawTx payload is too large");

        try {
            auto bytes = hex_to_bytes(raw_tx);
            auto signed_tx = parse_signed_transaction_payload(bytes);
            const auto txid = to_hex(signed_tx.tx.txid());
            const auto err = chain_.add_tx(signed_tx);
            const auto reason = tx_error_to_string(err);
            if (err != TxError::None)
                return error_response(400, reason);

            broadcast_new_tx(signed_tx.tx);
            std::ostringstream os;
            os << "{\"txid\":\"" << txid << "\",\"status\":\"submitted\"}";
            return json_response(200, os.str());
        } catch (const std::exception& e) {
            return error_response(400, e.what());
        }
    });

    CROW_ROUTE(app_, "/api/<path>").methods(crow::HTTPMethod::OPTIONS)
    ([](const crow::request&, const std::string&) {
        return json_response(204, "");
    });

    CROW_ROUTE(app_, "/ws/events")
    .websocket(&app_)
    .onopen([this](crow::websocket::connection& connection) {
        {
            std::lock_guard lock(ws_mutex_);
            ws_connections_.insert(&connection);
        }
        connection.send_text("{\"type\":\"connected\"}");
    })
    .onclose([this](crow::websocket::connection& connection, const std::string&, uint16_t) {
            std::lock_guard lock(ws_mutex_);
            ws_connections_.erase(&connection);
        })
    .onmessage([](crow::websocket::connection& connection, const std::string& message, bool is_binary) {
        if (is_binary)
            return;
        if (message == "ping" || message == "{\"type\":\"ping\"}")
            connection.send_text("{\"type\":\"pong\"}");
    });
}
