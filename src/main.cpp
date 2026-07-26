#include "axis/chain.h"
#include "axis/net.h"
#include "axis/util.h"
#include "axis/web.h"

#include <sodium.h>

#include <exception>
#include <string>
#include <thread>
#include <utility>

int main() {
    if (sodium_init() < 0) {
        logging::err("sodium init failed");
        return 1;
    }
    try {
        Chain chain;
        WebServer web{chain, 8080};
        ServerEvents events{
            .on_tx_accepted = [&](const Transaction& tx) {
                web.broadcast_new_tx(tx);
            },
            .on_block_accepted = [&](const Block& block) {
                web.broadcast_new_block(block);
            },
        };
        Server server{chain, 8889, std::move(events)};

        std::thread web_thread{[&] {
            logging::info("http/websocket server starting on port 8080");
            web.run();
        }};

        logging::info("tcp server starting on port 8889");
        server.run();

        web.stop();
        if (web_thread.joinable())
            web_thread.join();
    } catch (const std::exception& e) {
        logging::err(std::string{"fatal: "} + e.what());
        return 1;
    }
    return 0;
}
