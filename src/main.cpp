#include "axis/chain.h"
#include "axis/net.h"
#include "axis/util.h"
#include <sodium.h>

int main() {
    if (sodium_init() < 0) {
        logging::err("sodium init failed");
        return 1;
    }
    try {
        Chain chain;
        Server server{chain};
        logging::info("server starting on port 9618");
        server.run();
    } catch (const std::exception& e) {
        logging::err(std::string{"fatal: "} + e.what());
        return 1;
    }
    return 0;
}
