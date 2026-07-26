#pragma once

#include "axis/chain.h"

#include <crow.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>

class WebServer {
public:
    WebServer(Chain& chain, uint16_t port = 8080);

    void run();
    void stop();

    void broadcast_new_tx(const Transaction& tx);
    void broadcast_new_block(const Block& block);

private:
    Chain& chain_;
    uint16_t port_;
    crow::SimpleApp app_;
    std::mutex ws_mutex_;
    std::unordered_set<crow::websocket::connection*> ws_connections_;

    void setup_routes();
    void broadcast_text(const std::string& message);
};
