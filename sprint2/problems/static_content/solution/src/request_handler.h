#pragma once
#include <boost/beast/http.hpp>
#include <optional>

#include "api.h"
#include "error_codes.h"
#include "headers.h"
#include "http_server.h"
#include "model.h"

namespace http_handler {

namespace http = boost::beast::http;

template <class Base, class T>
std::unique_ptr<Base> static inline MakeUnique(auto& arg, auto& arg2, auto arg3) {
    return std::unique_ptr<Base>(dynamic_cast<Base*>(new T(arg, arg2, arg3)));
}

class BasicRequestTypeHandler {
   public:
    BasicRequestTypeHandler() = delete;
    BasicRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper, std::string_view static_folder)
        : game_(game), api_keeper_(keeper), static_folder_(static_folder) {}

    virtual std::string_view GetMethodString() const = 0;
    virtual message_pack_t Handle(const StringRequest& req);

   protected:
    api::ApiProxyKeeper& api_keeper_;
    model::Game game_;
    std::string_view static_folder_;
};

class GetRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = "GET"sv;

   public:
    GetRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper, std::string_view static_folder)
        : BasicRequestTypeHandler(game, keeper, static_folder) {}
    GetRequestTypeHandler() = delete;

    message_pack_t Handle(const StringRequest& req) override;

    std::string_view GetMethodString() const override { return method_string_; }

   private:
    void redirectTarget(std::string_view target, message_pack_t& resp);
};

class HeadRequestTypeHandler : public GetRequestTypeHandler {
    static constexpr auto method_string_ = "HEAD"sv;

   public:
    HeadRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper, std::string_view static_folder)
        : GetRequestTypeHandler(game, keeper, static_folder) {}

    message_pack_t Handle(const StringRequest& req) override;

    std::string_view GetMethodString() const override { return method_string_; }
};

class PostRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = "POST"sv;

   public:
    PostRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper, std::string_view static_folder)
        : BasicRequestTypeHandler(game, keeper, static_folder) {}
    PostRequestTypeHandler() = delete;

    std::string_view GetMethodString() const override { return method_string_; }
};

class BadRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = ""sv;
    static constexpr auto body_content_ = "Invalid method"sv;

   public:
    BadRequestTypeHandler(model::Game& game, api::ApiProxyKeeper& keeper, std::string_view static_folder)
        : BasicRequestTypeHandler(game, keeper, static_folder) {}
    BadRequestTypeHandler() = delete;

    message_pack_t Handle(const StringRequest& req, ErrorCodes status, std::optional<std::string_view> custom_body = std::nullopt);

    std::string_view GetMethodString() const override { return method_string_; }
};

class RequestHandler {
   public:
    explicit RequestHandler(model::Game& game, api::ApiProxyKeeper& keeper, std::string_view static_folder = "");

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        send(HandleRequest(std::forward<decltype(req)>(req)));
    }

   private:
    message_pack_t HandleRequest(StringRequest&& req);

    void PreSettings(StringRequest& req);

    model::Game& game_;
    std::vector<std::unique_ptr<BasicRequestTypeHandler>> handlers_variants_;
    std::unique_ptr<BadRequestTypeHandler> bad_request_;
    std::string_view static_folder_;
};

}  // namespace http_handler
