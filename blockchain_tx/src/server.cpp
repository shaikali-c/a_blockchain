#include "server.h"

using namespace drogon;
using Callback = std::function<void(const HttpResponsePtr&)>;

void Server::getReq(const std::string& path, Json::Value data) {
    app().registerHandler(path, [data](const HttpRequestPtr& req, Callback&& callback) {
        callback(HttpResponse::newHttpJsonResponse(data));
    }, { Get });
}

void Server::postReq(const std::string& path, std::function<bool(
    const std::array<unsigned char, crypto_generichash_BYTES>& s,
    const std::array<unsigned char, crypto_generichash_BYTES>& r,
    uint64_t amount)> handler
) {
    app().registerHandler(path, [handler](const HttpRequestPtr& req, Callback&& callback) {
        auto body = req->getBody();

        auto jsonBody = req->getJsonObject();
        Json::Value response;


        for (const auto& tx : *jsonBody) {
            if (!tx.isObject()) continue;

            std::string sender_serialize = tx["sender"].asString();
            std::string receiver_serialize = tx["receiver"].asString();
            uint64_t amount = tx["amount"].asUInt64();

            std::array<unsigned char, crypto_generichash_BYTES> sender;
            std::array<unsigned char, crypto_generichash_BYTES> receiver;

            toBytes(sender_serialize, sender.data(), sender.size());
            toBytes(receiver_serialize, receiver.data(), receiver.size());

            handler(sender, receiver, amount);
        }

        response["status"] = "batch success";

        callback(HttpResponse::newHttpJsonResponse(response));
        }, { Post });
}
