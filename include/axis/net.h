#pragma once

#include "axis/chain.h"
#include <asio.hpp>
#include <memory>

enum class MsgType : uint16_t {
    GetUTXOs = 3,
    GetBlock,
    GetTransaction,
    GetUTXO,
    UTXOsResponse = 8,
    TransactionResponse,
    CreateTransaction = 12,
};

class Server {
public:
    Server(Chain& chain, uint16_t port = 8889);

    void run();

private:
    asio::io_context ctx_;
    asio::ip::tcp::acceptor acceptor_;
    Chain& chain_;

    void do_accept();
    asio::awaitable<void> handle_client(
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> handle_msg(
        MsgType type, std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> on_get_utxos(
        std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    asio::awaitable<void> on_create_tx(
        std::span<const uint8_t> payload,
        std::shared_ptr<asio::ip::tcp::socket> sock);

    static asio::awaitable<void> send(
        std::shared_ptr<asio::ip::tcp::socket> sock,
        MsgType type, std::span<const uint8_t> payload);

    static std::vector<uint8_t> serialize_utxo_response(
        const std::vector<OutPoint>& outpoints, uint64_t total);

    static std::vector<uint8_t> serialize_tx_response(
        TxError err, std::string_view reason);
};
