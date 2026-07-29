#pragma once

#include "axis/chain.h"

#include <asio.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

enum class MsgType : uint16_t {
    GetUTXOs = 0,
    GetBlock,
    GetTip,
    GetTransaction,
    GetUTXO,
    GetDifficulty,
    GetPool,
    CreateTransaction,
    CreateBlock,
    DifficultyResponse,
    TransactionResponse,
    CreateBlockResponse,
    UTXOsResponse,
    TipResponse,
    PoolResponse,
};

struct ServerEvents {
    std::function<void(const Transaction&)> on_tx_accepted;
    std::function<void(const Block&)> on_block_accepted;
};

class Server {
public:
    Server(Chain& chain, uint16_t port = 8889, ServerEvents events = {});

    void run();
    void stop();
    asio::io_context& get_executor() { return ctx_; }

private:
    asio::io_context ctx_;
    asio::ip::tcp::acceptor acceptor_;
    Chain& chain_;
    ServerEvents events_;

    void do_accept();
    asio::awaitable<void> handle_client(
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> handle_msg(
        MsgType type, std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> on_get_utxos(
        std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> on_get_difficulty(
        std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> on_get_tip(
        std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> on_get_pool(
        std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> on_create_tx(
        std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> on_create_block(
        std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    static asio::awaitable<void> send(
        std::shared_ptr<asio::ip::tcp::socket> sock,
        MsgType type, std::span<const uint8_t> payload);

    std::vector<uint8_t> serialize_utxo_response(
        const std::vector<std::pair<OutPoint, uint64_t>>& outpoints);

    static std::vector<uint8_t> serialize_tx_response(
        TxError err, std::string_view reason);
};
