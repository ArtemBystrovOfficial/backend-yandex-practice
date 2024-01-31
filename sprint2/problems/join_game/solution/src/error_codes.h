#pragma once

////////////////////////////
//// Централизация ошибок
////////////////////////////

#include <optional>
#include <string>

#include "headers.h"

namespace http_handler {

using error_code_t = char;

enum ErrorCode {
    OK = 0x0,
    MAP_NOT_FOUNDED = 0x1,
    BAD_REQUEST = 0x2,
    BAD_ACCESS = 0x3,
    READ_FILE = 0x4,
    UNKNOWN_ERROR = 0x5,
    FILE_NOT_EXIST = 0x6,
    JOIN_PLAYER_MAP = 0x7,
    JOIN_PLAYER_NAME = 0x8,
    JOIN_PLAYER_UNKNOWN = 0x9,
    POST_NOT_ALLOWED = 0x10,
    GET_NOT_ALLOWED = 0x11,
    AUTHORIZATION_NOT_EXIST = 0x12,
    AUTHORIZATION_NOT_FOUND = 0x13
};

void FillInfoError(StringResponse& resp, const ErrorCode& code, std::optional<std::string_view> custom_body = std::nullopt);

}  // namespace http_handler
