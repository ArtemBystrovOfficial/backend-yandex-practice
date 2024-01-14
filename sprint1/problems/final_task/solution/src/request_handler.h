#pragma once
#include "api.h"
#include "error_codes.h"
#include "http_server.h"
#include "model.h"

namespace http_handler {

using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

namespace {
void FillBody(StringResponse& resp, std::string_view text) {
    resp.body() = text;
    resp.content_length(text.size());
}

template <class Base, class T>
std::unique_ptr<Base> static inline MakeUnique(auto& arg) {
    return std::unique_ptr<Base>(dynamic_cast<Base*>(new T(arg)));
}

// TODO макрос по переводу string view не работает
inline std::string_view ToSV(boost::beast::string_view bsv) {
    return std::string_view(bsv.data(), bsv.size());
}

std::vector<std::string_view> SplitUrl(std::string_view url) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    const size_t pos_end = url.npos;
    while (true) {
        size_t space = url.find('/', pos);
        auto str =
            space == pos_end ? url.substr(pos) : url.substr(pos, space - pos);
        if (!str.empty()) result.push_back(std::move(str));
        if (space == pos_end)
            break;
        else
            pos = space + 1;
    }
    return result;
}

}  // namespace

class BasicRequestTypeHandler {
   protected:
    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view JSON = "application/json"sv;
        constexpr static std::string_view ONLY_READ_ALLOW = "GET, HEAD"sv;
        static constexpr auto API_TYPE = "api"sv;
        static constexpr auto VERSION_1 = "v1"sv;
    };

   public:
    BasicRequestTypeHandler() = delete;
    BasicRequestTypeHandler(model::Game& game) : game_(game) {}

    virtual std::string_view MethodString() const = 0;
    virtual StringResponse Handle(const StringRequest& req) {
        StringResponse response(http::status::ok, req.version());
        response.set(http::field::content_type, ContentType::TEXT_HTML);
        FillBody(response, "");
        response.keep_alive(req.keep_alive());
        return response;
    }

   protected:
    model::Game game_;
};

class HeadRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = "HEAD"sv;

   public:
    HeadRequestTypeHandler(model::Game& game) : BasicRequestTypeHandler(game) {}

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
    GetRequestTypeHandler(model::Game& game) : HeadRequestTypeHandler(game) {}
    GetRequestTypeHandler() = delete;

    virtual StringResponse Handle(const StringRequest& req) {
        auto resp = HeadRequestTypeHandler::Handle(req);
        FillBody(resp, redirectTarget(ToSV(req.target()), resp));
        return resp;
    }

    virtual std::string_view MethodString() const override {
        return method_string_;
    }

   private:
    std::string redirectTarget(std::string_view target, StringResponse& resp) {
        auto values = SplitUrl(target);
        if (!values.empty()) {
            // API
            if (values.front() == ContentType::API_TYPE) {
                if (values.size() >= 2) {
                    auto api = api::API::GetAPI(values[1]);

                    // MAPS
                    if (values.size() >= 3) {
                        if (values[2] == "maps"sv) {
                            if (values.size() == 4) {
                                resp.set(http::field::content_type,
                                         ContentType::JSON);
                                return api->getMapDescriptionJson(
                                    game_, std::string{values[3]});
                            } else if (values.size() == 3) {
                                resp.set(http::field::content_type,
                                         ContentType::JSON);
                                return api->getMapListJson(game_);
                            }
                        }
                    }
                }
            }
        }
        throw ErrorCodes::BAD_REQUEST;
    }
};

class PostRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = "POST"sv;

   public:
    PostRequestTypeHandler(model::Game& game) : BasicRequestTypeHandler(game) {}
    PostRequestTypeHandler() = delete;

    virtual std::string_view MethodString() const override {
        return method_string_;
    }
};

class BadRequestTypeHandler : public BasicRequestTypeHandler {
    static constexpr auto method_string_ = ""sv;
    static constexpr auto body_content_ = "Invalid method"sv;

   public:
    BadRequestTypeHandler(model::Game& game) : BasicRequestTypeHandler(game) {}
    BadRequestTypeHandler() = delete;

    StringResponse Handle(const StringRequest& req, ErrorCodes status) {
        auto resp = BasicRequestTypeHandler::Handle(req);
        resp.set(http::field::content_type, ContentType::JSON);

        std::string_view code = "";
        std::string_view message = "";

        switch (status) {
            case ErrorCodes::BAD_REQUEST:
                resp.result(http::status::bad_request);
                code = "badRequest"sv;
                message = "Bad request"sv;  // TODO Все коды и сообщения ошибок
                                            // вынести в error_codes.h
                break;
            case ErrorCodes::MAP_NOT_FOUNDED:
                resp.result(http::status::not_found);
                code = "mapNotFound"sv;
                message = "Map not found"sv;
                break;
        }

        ptree tree;
        tree.put("code", code);
        tree.put("message", message);

        FillBody(resp, json_loader::JsonObject::GetJson(tree));
        return resp;
    }

    virtual std::string_view MethodString() const override {
        return method_string_;
    }
};

class RequestHandler {
   public:
    explicit RequestHandler(model::Game& game)
        : game_{game},
          bad_request_(
              MakeUnique<BadRequestTypeHandler, BadRequestTypeHandler>(game)) {
        handlers_variants_.push_back(
            MakeUnique<BasicRequestTypeHandler, HeadRequestTypeHandler>(game));
        handlers_variants_.push_back(
            MakeUnique<BasicRequestTypeHandler, GetRequestTypeHandler>(game));
        handlers_variants_.push_back(
            MakeUnique<BasicRequestTypeHandler, PostRequestTypeHandler>(game));
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
            return bad_request_->Handle(req, ErrorCodes::BAD_REQUEST);

        try {
            auto resp = (*handler)->Handle(req);
            return resp;
        } catch (const ErrorCodes& ec) {
            auto resp = bad_request_->Handle(req, ec);
            return resp;
        } catch (const std::exception& ec) {
            auto resp = bad_request_->Handle(req, ErrorCodes::BAD_REQUEST);
            return resp;
        }
    }

    model::Game& game_;
    std::vector<std::unique_ptr<BasicRequestTypeHandler>> handlers_variants_;
    std::unique_ptr<BadRequestTypeHandler> bad_request_;
};

}  // namespace http_handler
