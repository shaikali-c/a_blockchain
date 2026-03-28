#include "network.h"

using namespace drogon;
using Callback = std::function<void(const HttpResponsePtr&)>;

void Network::getReq(const std::string& path, const Json::Value& data) {
    app().registerHandler(path, [data](const HttpRequestPtr& req, Callback&& callback) {
        callback(HttpResponse::newHttpJsonResponse(data));
    }, { Get });
}
