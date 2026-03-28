#pragma once
#include <drogon/drogon.h>

class Network {
public:
    static Network& getInstance() {
        static Network instance;
        return instance;
    }

    void start() {
        if (started) return;
        started = true;
        drogon::app().addListener("0.0.0.0", 8080);
        drogon::app().run();
    }

    void getReq(const std::string& path, const Json::Value& data);

private:
    Network() = default;
    ~Network() = default;

    Network(const Network&) = delete;
    Network& operator=(const Network&) = delete;

    bool started = false;
};