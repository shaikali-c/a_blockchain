#include "network.h"

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

        if (jsonBody) {
            std::string sender_serialize = (*jsonBody)["sender"].asString();
            std::string receiver_serialize = (*jsonBody)["receiver"].asString();
            uint64_t amount = (*jsonBody)["amount"].asInt64();

            std::array<unsigned char, crypto_generichash_BYTES> sender;
            std::array<unsigned char, crypto_generichash_BYTES> receiver;

            toBytes(sender_serialize, sender.data(), sender.size());
            toBytes(receiver_serialize, receiver.data(), receiver.size());

            bool validTX = handler(sender, receiver, amount);
            if (validTX) {
                response["status"] = "success";
            }
            else {
                response["status"] = "Not Enough Coins";
            }
        }
        else {
            response["error"] = "Invalid JSON body";
        }

        callback(HttpResponse::newHttpJsonResponse(response));
        }, { Post });
}
