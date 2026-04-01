#pragma once
#include "pch.h"
#include "common.h"
#include <sodium.h>

class Server {
public:
    static Server& getInstance() {
        static Server instance;
        return instance;
    }

    void start() {
        if (started) return;
        started = true;
        drogon::app().addListener("0.0.0.0", 8080);
        drogon::app().run();
    }

    void getReq(const std::string& path, Json::Value data);
    void postReq(const std::string& path, std::function<bool(const std::array<unsigned char, crypto_generichash_BYTES>& s, const std::array<unsigned char, crypto_generichash_BYTES>& r, uint64_t amount)>);


private:
    Server() = default;
    ~Server() = default;

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    bool started = false;
};