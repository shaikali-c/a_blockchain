#include "axis/web.h"

#include "axis/util.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <exception>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using json = nlohmann::json;

constexpr uint32_t kDefaultBlockCount = 10;
constexpr uint32_t kMaxBlockCount = 100;
constexpr size_t kMaxRawTxHexChars = 128 * 1024;

constexpr size_t kOutPointSize = 36;   // hash(32) + index(4)
constexpr size_t kTxOutputSize = 28;   // addr(20) + amount(8)
constexpr size_t kTxFixedOverhead = 48; // txid(32) + timestamp(8) + in_count(4) + out_count(4)
constexpr size_t kBlockHeaderSize = 80; // prev_hash(32) + merkle_root(32) + timestamp(8) + nonce(8)

crow::response json_response(int code, json body) {
    crow::response res{code, body.dump()};
    res.set_header("Content-Type", "application/json");
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, X-API-Key");
    return res;
}

crow::response error_response(int code, std::string_view message) {
    json j = {{"error", message}, {"code", code}};
    return json_response(code, std::move(j));
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

json outpoint_json(const OutPoint& outpoint) {
    return {{"txid", to_hex(outpoint.txid)}, {"index", outpoint.index}, {"size", kOutPointSize}};
}

json output_json(const TxOutput& output) {
    return {{"recipient", to_hex(output.recipient)}, {"amount", output.amount}, {"size", kTxOutputSize}};
}

json transaction_json(const Transaction& tx) {
    json j;
    j["txid"] = to_hex(tx.txid());
    j["timestamp"] = tx.timestamp.value;
    j["coinbase"] = tx.is_coinbase();
    j["size"] = kTxFixedOverhead + kOutPointSize * tx.inputs.size() + kTxOutputSize * tx.outputs.size();
    j["inputs"] = json::array();
    for (const auto& in : tx.inputs)
        j["inputs"].push_back(outpoint_json(in));
    j["outputs"] = json::array();
    for (const auto& out : tx.outputs)
        j["outputs"].push_back(output_json(out));
    return j;
}

size_t block_serialized_size(const Block& block) {
    size_t s = kBlockHeaderSize + 4; // header + tx count
    for (const auto& tx : block.transactions) {
        size_t tx_size = kTxFixedOverhead + kOutPointSize * tx.inputs.size() + kTxOutputSize * tx.outputs.size();
        s += 4 + tx_size; // size prefix + tx payload
    }
    return s;
}

json block_summary_json(const Block& block, uint32_t height) {
    json transactionsJson;
    for (const auto& tx : block.transactions)
        transactionsJson.push_back(transaction_json(tx));
    return {
        {"height", height},
        {"hash", to_hex(block.hash())},
        {"previousHash", to_hex(block.header().prev_hash)},
        {"merkleRoot", to_hex(block.header().merkle_root)},
        {"timestamp", block.header().timestamp.value},
        {"nonce", block.header().nonce},
        {"transactionCount", block.transactions.size()},
        {"transactions", transactionsJson},
        {"size", block_serialized_size(block)}
    };
}

json block_json(const Block& block, uint32_t height) {
    json j;
    j["height"] = height;
    j["hash"] = to_hex(block.hash());
    j["previousHash"] = to_hex(block.header().prev_hash);
    j["merkleRoot"] = to_hex(block.header().merkle_root);
    j["timestamp"] = block.header().timestamp.value;
    j["nonce"] = block.header().nonce;
    j["size"] = block_serialized_size(block);
    j["txids"] = json::array();
    for (const auto& tx : block.transactions)
        j["txids"].push_back(to_hex(tx.txid()));
    j["transactions"] = json::array();
    for (const auto& tx : block.transactions)
        j["transactions"].push_back(transaction_json(tx));
    return j;
}

json event_new_tx_json(const Transaction& tx) {
    return {
        {"type", "new_tx"},
        {"txid", to_hex(tx.txid())},
        {"size", kTxFixedOverhead + kOutPointSize * tx.inputs.size() + kTxOutputSize * tx.outputs.size()},
        {"transaction", transaction_json(tx)}
    };
}

json event_new_block_json(const Block& block) {
    return {
        {"type", "new_block"},
        {"hash", to_hex(block.hash())},
        {"timestamp", block.header().timestamp.value},
        {"transactionCount", block.transactions.size()},
        {"size", block_serialized_size(block)}
    };
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
    broadcast_text(event_new_tx_json(tx).dump());
}

void WebServer::broadcast_new_block(const Block& block) {
    broadcast_text(event_new_block_json(block).dump());
}

void WebServer::broadcast_text(const std::string& message) {
    std::lock_guard lock(ws_mutex_);
    for (auto* connection : ws_connections_)
        connection->send_text(message);
}

void WebServer::setup_routes() {
    CROW_ROUTE(app_, "/api/status").methods(crow::HTTPMethod::GET)
    ([this]() {
        json j = {
            {"status", "online"},
            {"version", "0.1"},
            {"blockHeight", chain_.height()},
            {"difficulty", static_cast<uint32_t>(chain_.get_difficulty())},
            {"tipHash", to_hex(chain_.tip_hash())}
        };
        return json_response(200, std::move(j));
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
        json j;
        j["blocks"] = json::array();
        for (size_t i = 0; i < blocks.size(); ++i)
            j["blocks"].push_back(block_summary_json(blocks[i], start + static_cast<uint32_t>(i)));
        j["total"] = chain_.height();
        return json_response(200, std::move(j));
    });

    CROW_ROUTE(app_, "/api/mempool").methods(crow::HTTPMethod::GET)
    ([this]() {
        const auto txs = chain_.get_pool_txs();
        json j;
        j["size"] = txs.size();
        j["txids"] = json::array();
        for (const auto& tx : txs)
            j["txids"].push_back(to_hex(tx.txid()));
        j["transactions"] = json::array();
        for (const auto& tx : txs)
            j["transactions"].push_back(transaction_json(tx));
        return json_response(200, std::move(j));
    });

    CROW_ROUTE(app_, "/api/charts").methods(crow::HTTPMethod::GET)
    ([this]() {
        const auto height = chain_.height();
        json j;

        // blocksOverTime — group blocks by hour for the last 24 slots
        j["blocksOverTime"] = json::array();
        if (height > 0) {
            const auto now = static_cast<uint64_t>(std::time(nullptr));
            constexpr uint64_t kHour = 3600;
            uint64_t current_hour = now - (now % kHour);
            std::map<uint64_t, uint32_t> hour_counts;
            for (uint32_t h = 0; h < height; ++h) {
                auto block = chain_.get_block(h);
                if (!block) continue;
                uint64_t bh = block->header().timestamp.value - (block->header().timestamp.value % kHour);
                if (bh > current_hour - 23 * kHour)
                    hour_counts[bh]++;
            }
            for (uint64_t h = current_hour - 23 * kHour; h <= current_hour; h += kHour) {
                char buf[6];
                std::tm tm{};
                tm.tm_sec = 0; tm.tm_min = 0; tm.tm_hour = 0;
                std::time_t t = static_cast<std::time_t>(h);
                gmtime_r(&t, &tm);
                std::strftime(buf, sizeof(buf), "%H:%M", &tm);
                j["blocksOverTime"].push_back({{"time", buf}, {"blocksMined", hour_counts[h]}});
            }
        }

        // txPerBlock — last 20 blocks
        j["txPerBlock"] = json::array();
        {
            const auto now = static_cast<uint64_t>(std::time(nullptr));
            uint32_t start = height > 20 ? height - 20 : 0;
            for (uint32_t h = start; h < height; ++h) {
                auto block = chain_.get_block(h);
                if (!block) continue;
                uint64_t dt = now - block->header().timestamp.value;
                std::string rel;
                if (dt < 60)      rel = std::to_string(dt) + "s ago";
                else if (dt < 3600) rel = std::to_string(dt / 60) + "m ago";
                else                rel = std::to_string(dt / 3600) + "h ago";
                j["txPerBlock"].push_back({
                    {"blockHeight", "#" + std::to_string(h)},
                    {"txCount", static_cast<uint64_t>(block->transactions.size())},
                    {"time", rel}
                });
            }
        }

        // avgBlockSize — group by hour (same buckets as blocksOverTime)
        j["avgBlockSize"] = json::array();
        if (height > 0) {
            const auto now = static_cast<uint64_t>(std::time(nullptr));
            constexpr uint64_t kHour = 3600;
            uint64_t current_hour = now - (now % kHour);
            std::map<uint64_t, std::pair<uint64_t, uint32_t>> hour_sizes;
            for (uint32_t h = 0; h < height; ++h) {
                auto block = chain_.get_block(h);
                if (!block) continue;
                uint64_t bh = block->header().timestamp.value - (block->header().timestamp.value % kHour);
                if (bh > current_hour - 23 * kHour) {
                    hour_sizes[bh].first += block_serialized_size(*block);
                    hour_sizes[bh].second++;
                }
            }
            for (uint64_t h = current_hour - 23 * kHour; h <= current_hour; h += kHour) {
                char buf[6];
                std::tm tm{};
                std::time_t t = static_cast<std::time_t>(h);
                gmtime_r(&t, &tm);
                std::strftime(buf, sizeof(buf), "%H:%M", &tm);
                auto& [total, count] = hour_sizes[h];
                j["avgBlockSize"].push_back({
                    {"time", buf},
                    {"avgSize", count > 0 ? static_cast<uint64_t>(total / count) : 0}
                });
            }
        }

        // networkActivity — TPS from recent blocks (last 10)
        j["networkActivity"] = json::array();
        {
            const auto now = static_cast<uint64_t>(std::time(nullptr));
            uint32_t start = height > 10 ? height - 10 : 0;
            uint64_t total_tx = 0;
            uint64_t time_span = 0;
            for (uint32_t h = start; h < height; ++h) {
                auto block = chain_.get_block(h);
                if (!block) continue;
                total_tx += block->transactions.size();
                if (h == start) {
                    time_span = now - block->header().timestamp.value;
                }
            }
            // Show 3 time windows: 1m, 5m, 1h
            const std::pair<const char*, uint64_t> windows[] = {
                {"1m ago", 60}, {"5m ago", 300}, {"1h ago", 3600}
            };
            for (const auto& [label, secs] : windows) {
                uint64_t recent_tx = 0;
                for (uint32_t h = height; h > 0; --h) {
                    auto block = chain_.get_block(h - 1);
                    if (!block) continue;
                    if (now - block->header().timestamp.value > secs) break;
                    recent_tx += block->transactions.size();
                }
                j["networkActivity"].push_back({
                    {"time", label},
                    {"tps", secs > 0 ? static_cast<double>(recent_tx) / static_cast<double>(secs) : 0.0}
                });
            }
        }

        return json_response(200, std::move(j));
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

        json j;
        j["address"] = to_hex(address);
        uint64_t balance = 0;
        j["utxos"] = json::array();
        for (const auto& [outpoint, amount] : utxos) {
            balance += amount;
            j["utxos"].push_back({
                {"txid", to_hex(outpoint.txid)},
                {"index", outpoint.index},
                {"amount", amount},
                {"size", kOutPointSize}
            });
        }
        j["balance"] = balance;
        return json_response(200, std::move(j));
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
            json j = {{"txid", txid}, {"status", "submitted"}};
            return json_response(200, std::move(j));
        } catch (const std::exception& e) {
            return error_response(400, e.what());
        }
    });

    CROW_ROUTE(app_, "/api/<path>").methods(crow::HTTPMethod::OPTIONS)
    ([](const crow::request&, const std::string&) {
        return json_response(204, json{});
    });

    CROW_ROUTE(app_, "/ws/events")
    .websocket(&app_)
    .onopen([this](crow::websocket::connection& connection) {
        {
            std::lock_guard lock(ws_mutex_);
            ws_connections_.insert(&connection);
        }
        connection.send_text(R"({"type":"connected"})");
    })
    .onclose([this](crow::websocket::connection& connection, const std::string&, uint16_t) {
            std::lock_guard lock(ws_mutex_);
            ws_connections_.erase(&connection);
        })
    .onmessage([](crow::websocket::connection& connection, const std::string& message, bool is_binary) {
        if (is_binary)
            return;
        if (message == "ping" || message == R"({"type":"ping"})")
            connection.send_text(R"({"type":"pong"})");
    });
}
