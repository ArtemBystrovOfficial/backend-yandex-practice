#include "request_handler.h"

namespace http_handler {

namespace {

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view JSON = "application/json"sv;
    constexpr static std::string_view ONLY_READ_ALLOW = "GET, HEAD"sv;
    static constexpr auto API_TYPE = "api"sv;
    static constexpr auto VERSION_1 = "v1"sv;
};

void FillBody(StringResponse& resp, std::string_view text) {
    resp.body() = text;
    resp.content_length(text.size());
}

void FillContentType(StringResponse& resp, api::ApiCommon::TypeData type) {
    std::string content;
    switch (type) {
        case api::ApiCommon::TypeData::HTML:
            content = ContentType::TEXT_HTML;
        case api::ApiCommon::TypeData::JSON:
            content = ContentType::JSON;
    }
    resp.set(http::field::content_type, content);
}

template <class Base, class T>
std::unique_ptr<Base> static inline MakeUnique(auto& arg, auto& arg2) {
    return std::unique_ptr<Base>(dynamic_cast<Base*>(new T(arg, arg2)));
}

// TODO макрос по переводу string view не работает
inline std::string_view ToSV(boost::beast::string_view bsv) { return std::string_view(bsv.data(), bsv.size()); }

std::deque<std::string_view> SplitUrl(std::string_view url) {
    std::deque<std::string_view> result;

    size_t pos = 0;
    const size_t pos_end = url.npos;
    while (true) {
        size_t space = url.find('/', pos);
        auto str = space == pos_end ? url.substr(pos) : url.substr(pos, space - pos);
        if (!str.empty()) result.push_back(std::move(str));
        if (space == pos_end)
            break;
        else
            pos = space + 1;
    }
    return result;
}

}  // namespace

StringResponse BasicRequestTypeHandler::Handle(const StringRequest& req) {
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, ContentType::TEXT_HTML);
    FillBody(response, "");
    response.keep_alive(req.keep_alive());
    return response;
}

StringResponse GetRequestTypeHandler::Handle(const StringRequest& req) {
    auto resp = HeadRequestTypeHandler::Handle(req);
    FillBody(resp, redirectTarget(ToSV(req.target()), resp));
    return resp;
}

std::string GetRequestTypeHandler::redirectTarget(std::string_view target, StringResponse& resp) {
    auto values = SplitUrl(target);
    if (!values.empty()) {
        auto type = values.front();
        // API
        if (type == ContentType::API_TYPE) {
            values.pop_front();
            if (!values.empty()) {
                auto version = values.front();
                values.pop_front();
                auto api_ptr = api_keeper_.GetApiByVersion(version);
                auto [data, content_type] = api_ptr->GetFormatData(std::move(values), resp.body());
                FillContentType(resp, content_type);
                return data;
            }
        }
    }
    throw ErrorCodes::BAD_REQUEST;
}

StringResponse HeadRequestTypeHandler::Handle(const StringRequest& req) { return BasicRequestTypeHandler::Handle(req); }
StringResponse BadRequestTypeHandler::Handle(const StringRequest& req, ErrorCodes status) {
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
RequestHandler::RequestHandler(model::Game& game, api::ApiProxyKeeper& keeper)
    : game_{game}, bad_request_(MakeUnique<BadRequestTypeHandler, BadRequestTypeHandler>(game, keeper)) {
    handlers_variants_.push_back(MakeUnique<BasicRequestTypeHandler, HeadRequestTypeHandler>(game, keeper));
    handlers_variants_.push_back(MakeUnique<BasicRequestTypeHandler, GetRequestTypeHandler>(game, keeper));
    handlers_variants_.push_back(MakeUnique<BasicRequestTypeHandler, PostRequestTypeHandler>(game, keeper));
}
StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
    auto handler = std::find_if(handlers_variants_.begin(), handlers_variants_.end(),
                                [&req](const auto& handler) { return handler->GetMethodString() == req.method_string(); });
    if (handler == handlers_variants_.end()) return bad_request_->Handle(req, ErrorCodes::BAD_REQUEST);

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
}  // namespace http_handler
