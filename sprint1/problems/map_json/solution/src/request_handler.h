#pragma once
#include "http_server.h"
#include "model.h"

namespace http_handler {

using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

namespace {
void FillBody(StringResponse& resp, const std::string& text) {
    resp.body() = text;
    resp.content_length(text.size());
}

template <class Base, class T>
std::unique_ptr<Base> static inline MakeUnique() {
    return std::unique_ptr<Base>(dynamic_cast<Base*>(new T));
}

}  // namespace

class BasicRequestTypeHandler {
   protected:
    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view ONLY_READ_ALLOW = "GET, HEAD"sv;
    };

   public:
    virtual std::string_view MethodString() const = 0;
    virtual StringResponse Handle(const StringRequest& req) {
        StringResponse response(http::status::ok, req.version());
        response.set(http::field::content_type, ContentType::TEXT_HTML);
        FillBody(response, "");
        response.keep_alive(req.keep_alive());
        return response;
    }

   private:
};

class HeadRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = "HEAD"sv;

   public:
    HeadRequestTypeHandler() = default;

    virtual StringResponse Handle(const StringRequest& req) {
        return BasicRequestTypeHandler::Handle(req);
    }

    virtual std::string_view MethodString() const override {
        return method_string_;
    }
};

class GetRequestTypeHandler : public HeadRequestTypeHandler {
    static constexpr auto method_string_ = "GET"sv;

   public:
    GetRequestTypeHandler() = default;

    virtual StringResponse Handle(const StringRequest& req) {
        auto resp = HeadRequestTypeHandler::Handle(req);
        FillBody(resp, "Test");
        return resp;
    }

    virtual std::string_view MethodString() const override {
        return method_string_;
    }
};

class PostRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = "POST"sv;

   public:
    PostRequestTypeHandler() = default;

    virtual std::string_view MethodString() const override {
        return method_string_;
    }
};

class BadRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = ""sv;
    static constexpr auto body_content_ = "Invalid method"s;

   public:
    BadRequestTypeHandler() = default;

    virtual StringResponse Handle(const StringRequest& req) {
        auto resp = BasicRequestTypeHandler::Handle(req);
        resp.result(http::status::method_not_allowed);
        resp.set(http::field::allow, ContentType::ONLY_READ_ALLOW);
        FillBody(resp, body_content_);
        return resp;
    }

    virtual std::string_view MethodString() const override {
        return method_string_;
    }
};

class RequestHandler {
   public:
    explicit RequestHandler(model::Game& game) : game_{game} {
        handlers_variants_.push_back(
            MakeUnique<BasicRequestTypeHandler, BadRequestTypeHandler>());
        handlers_variants_.push_back(
            MakeUnique<BasicRequestTypeHandler, HeadRequestTypeHandler>());
        handlers_variants_.push_back(
            MakeUnique<BasicRequestTypeHandler, GetRequestTypeHandler>());
        handlers_variants_.push_back(
            MakeUnique<BasicRequestTypeHandler, PostRequestTypeHandler>());
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send) {
        send(HandleRequest(std::forward<decltype(req)>(req)));
    }

   private:
    StringResponse HandleRequest(StringRequest&& req) {
        auto handler = std::find_if(
            handlers_variants_.begin(), handlers_variants_.end(),
            [&req](const auto& handler) {
                return handler->MethodString() == req.method_string();
            });
        if (handler == handlers_variants_.end())
            handler = handlers_variants_.begin();  // ERROR ALWAYS FIRST
        return (*handler)->Handle(req);
    }

    model::Game& game_;
    std::vector<std::unique_ptr<BasicRequestTypeHandler>> handlers_variants_;
};

}  // namespace http_handler
