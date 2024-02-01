#pragma once
#include <boost/beast/http.hpp>
#include <optional>

#include "api.h"
#include "error_codes.h"
#include "headers.h"
#include "request_redirection.h"

namespace http_handler {

namespace http = boost::beast::http;
using RedirectionPack = std::map<std::string_view, std::shared_ptr<BasicRedirection>>;

template <class Base, class T, class... Args>
std::unique_ptr<Base> static inline MakeUnique(Args&&... args) {
    return std::unique_ptr<Base>(dynamic_cast<Base*>(new T(std::forward<Args>(args)...)));
}

class BasicRequestTypeHandler {
   public:
    BasicRequestTypeHandler() = delete;
    BasicRequestTypeHandler(api::ApiProxyKeeper& keeper, std::string_view static_folder);

    virtual message_pack_t Handle(const StringRequest& req);

    std::shared_ptr<BasicRedirection> ExtractRequestRedirection(Args_t& args);

   protected:
    api::ApiProxyKeeper& api_keeper_;
    std::string_view static_folder_;

   private:
    RedirectionPack redirection_pack_;
    std::shared_ptr<FilesystemRedirection> default_redirection_;
};

class BadRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = ""sv;
    static constexpr auto body_content_ = "Invalid method"sv;

   public:
    BadRequestTypeHandler(api::ApiProxyKeeper& keeper, std::string_view static_folder) : BasicRequestTypeHandler(keeper, static_folder) {}
    BadRequestTypeHandler() = delete;

    message_pack_t Handle(const StringRequest& req, ErrorCode status, std::optional<std::string_view> custom_body = std::nullopt);
};

class RequestHandler {
   public:
    explicit RequestHandler(api::ApiProxyKeeper& keeper, std::string_view static_folder = "");

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        send(HandleRequest(std::forward<decltype(req)>(req)));
    }

   private:
    message_pack_t HandleRequest(StringRequest&& req);
    void PreSettings(StringRequest& req);

    BasicRequestTypeHandler basic_request_;
    BadRequestTypeHandler bad_request_;
    std::string_view static_folder_;
};

}  // namespace http_handler
