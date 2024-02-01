#include "error_codes.h"

#include <bitset>

#include "common.h"
#include "json_loader.h"

namespace {
namespace http = boost::beast::http;
}

namespace http_handler {
ErrorCode& operator|=(ErrorCode& lhs, const ErrorCode& rhs) {
    using underlying_t = std::underlying_type_t<ErrorCode>;
    lhs = static_cast<ErrorCode>(static_cast<underlying_t>(lhs) | static_cast<underlying_t>(rhs));
    return lhs;
}

// TODO придумать более обощенную обработку ошибок
void FillInfoError(StringResponse& resp, ErrorCode ec_code, std::optional<std::string_view> custom_body) {
    std::string_view code = "";
    std::string_view message = "";

    std::optional<std::string> allow_access;

    if (ec_code > ErrorCode::NOT_ALLOWED) {
        std::string str;
        if (ec_code & POST_NOT_ALLOWED) str += "POST, ";
        if (ec_code & GET_ALLOWED) str += "GET, ";
        if (ec_code & DELETE_ALLOWED) str += "DELETE, ";
        if (ec_code & PUT_ALLOWED) str += "PUT, ";
        if (ec_code & OPTIONS_ALLOWED) str += "OPTIONS, ";
        if (ec_code & HEAD_ALLOWED) str += "HEAD, ";
        if (ec_code & PATCH_ALLOWED) str += "PATCH, ";

        str.erase(str.end() - 2, str.end());
        allow_access = str;
        ec_code = ErrorCode::NOT_ALLOWED;
    }

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
            resp.result(http::status::bad_request);
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
        case ErrorCode::NOT_ALLOWED:
            resp.result(http::status::method_not_allowed);
            resp.set(http::field::cache_control, "no-cache");
            if (allow_access) resp.set(http::field::allow, *allow_access);
            code = "invalidMethod"sv;
            message = *allow_access;
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

    resp.set(http::field::content_type, ToBSV(ContentType::JSON));
    ptree tree;
    tree.put("code", code);
    tree.put("message", message);

    common_pack::FillBody(resp, json_loader::JsonObject::GetJson(tree));
}
}  // namespace http_handler