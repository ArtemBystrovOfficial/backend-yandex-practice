#include "error_codes.h"

#include "common.h"
#include "json_loader.h"

namespace {
namespace http = boost::beast::http;
}

namespace http_handler {

// TODO придумать более обощенную обработку ошибок
void FillInfoError(StringResponse& resp, const ErrorCode& ec_code, std::optional<std::string_view> custom_body) {
    std::string_view code = "";
    std::string_view message = "";

    switch (ec_code) {
        case ErrorCode::BAD_REQUEST:
            resp.result(http::status::bad_request);
            code = "badRequest"sv;
            message = "badRequest"sv;
            break;
        case ErrorCode::MAP_NOT_FOUNDED:
            resp.result(http::status::not_found);
            code = "mapNotFound"sv;
            message = "mapNotFound"sv;
            break;
        case ErrorCode::BAD_ACCESS:
            resp.result(http::status::not_found);
            code = "insufficient permissions"sv;
            message = "insufficient permissions"sv;
            break;
        case ErrorCode::READ_FILE:
            resp.result(http::status::not_found);
            code = "read file error"sv;
            message = "read file error"sv;
            break;
        case ErrorCode::UNKNOWN_ERROR:
            resp.result(http::status::not_found);
            code = "unknown"sv;
            if (custom_body) message = *custom_body;
            break;
        case ErrorCode::JOIN_PLAYER_MAP:
            resp.result(http::status::not_found);
            resp.set(http::field::cache_control, "no-cache");
            code = "mapNotFound"sv;
            message = "Map not found"sv;
            break;
        case ErrorCode::JOIN_PLAYER_NAME:
            resp.result(http::status::not_found);
            resp.set(http::field::cache_control, "no-cache");
            code = "invalidArgument"sv;
            message = "Invalid name"sv;
            break;
        case ErrorCode::JOIN_PLAYER_UNKNOWN:
            resp.result(http::status::bad_request);
            resp.set(http::field::cache_control, "no-cache");
            code = "invalidArgument"sv;
            message = "Join game request parse error"sv;
            break;
        case ErrorCode::GET_NOT_ALLOWED:
            resp.result(http::status::method_not_allowed);
            resp.set(http::field::cache_control, "no-cache");
            code = "invalidMethod"sv;
            message = "Only POST method is expected"sv;
            break;
        case ErrorCode::POST_NOT_ALLOWED:
            resp.result(http::status::method_not_allowed);
            resp.set(http::field::cache_control, "no-cache");
            code = "invalidMethod"sv;
            message = "Only GET method is expected"sv;
            break;
        case ErrorCode::AUTHORIZATION_NOT_EXIST:
            resp.result(http::status::unauthorized);
            resp.set(http::field::cache_control, "no-cache");
            code = "invalidToken"sv;
            message = "Authorization header is missing"sv;
            break;
        case ErrorCode::AUTHORIZATION_NOT_FOUND:
            resp.result(http::status::unauthorized);
            resp.set(http::field::cache_control, "no-cache");
            code = "unknownToken"sv;
            message = "Player token has not been found"sv;
            break;
        case ErrorCode::FILE_NOT_EXIST:
            resp.result(http::status::not_found);
            resp.set(http::field::content_type, "text/plain");
            common_pack::FillBody(resp, "file not exist");
            return;
    }

    ptree tree;
    tree.put("code", code);
    tree.put("message", message);

    common_pack::FillBody(resp, json_loader::JsonObject::GetJson(tree));
}
}  // namespace http_handler