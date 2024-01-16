#pragma once
#include "api.h"
#include "error_codes.h"
#include "http_server.h"
#include "model.h"

namespace http_handler {

using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

using FileResponse = http::response<http::file_body>;

class BasicRequestTypeHandler {
   public:
    BasicRequestTypeHandler() = delete;
    BasicRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper) : game_(game), api_keeper_(keeper) {}

    virtual std::string_view GetMethodString() const = 0;
    virtual StringResponse Handle(const StringRequest& req);

   protected:
    api::ApiProxyKeeper& api_keeper_;
    model::Game game_;
};

class HeadRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = "HEAD"sv;

   public:
    HeadRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper) : BasicRequestTypeHandler(game, keeper) {}

    StringResponse Handle(const StringRequest& req) override;

    std::string_view GetMethodString() const override { return method_string_; }
};

class GetRequestTypeHandler : public HeadRequestTypeHandler {
    static constexpr auto method_string_ = "GET"sv;

   public:
    GetRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper) : HeadRequestTypeHandler(game, keeper) {}
    GetRequestTypeHandler() = delete;

    StringResponse Handle(const StringRequest& req) override;

    std::string_view GetMethodString() const override { return method_string_; }

   private:
    std::string redirectTarget(std::string_view target, StringResponse& resp);
};

class PostRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = "POST"sv;

   public:
    PostRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper) : BasicRequestTypeHandler(game, keeper) {}
    PostRequestTypeHandler() = delete;

    std::string_view GetMethodString() const override { return method_string_; }
};

class BadRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = ""sv;
    static constexpr auto body_content_ = "Invalid method"sv;

   public:
    BadRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper) : BasicRequestTypeHandler(game, keeper) {}
    BadRequestTypeHandler() = delete;

    StringResponse Handle(const StringRequest& req, ErrorCodes status);

    std::string_view GetMethodString() const override { return method_string_; }
};

class RequestHandler {
   public:
    explicit RequestHandler(model::Game& game, api::ApiProxyKeeper& keeper);

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        send(HandleRequest(std::forward<decltype(req)>(req)));
    }

   private:
    StringResponse HandleRequest(StringRequest&& req);

    model::Game& game_;
    std::vector<std::unique_ptr<BasicRequestTypeHandler>> handlers_variants_;
    std::unique_ptr<BadRequestTypeHandler> bad_request_;
};

}  // namespace http_handler
