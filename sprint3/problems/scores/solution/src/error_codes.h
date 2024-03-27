#pragma once

////////////////////////////
//// Централизация ошибок
////////////////////////////

#include <optional>
#include <string>

#include "headers.h"

namespace http_handler {

using error_code_t = char;

// TODO Переделать все ошибки в полиморфные классы
enum ErrorCode : uint32_t {
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
    AUTHORIZATION_NOT_EXIST = 0xa,
    AUTHORIZATION_NOT_FOUND = 0xb,
    BAD_REQUEST_TICK = 0xc,
    BAD_TICK_ACCESS = 0xd,

    NOT_ALLOWED = 1 << 8,
    POST_NOT_ALLOWED = 1 << 9,
    GET_ALLOWED = 1 << 10,
    DELETE_ALLOWED = 1 << 11,
    PUT_ALLOWED = 1 << 12,
    OPTIONS_ALLOWED = 1 << 13,
    HEAD_ALLOWED = 1 << 14,
    PATCH_ALLOWED = 1 << 15,
};

ErrorCode& operator|=(ErrorCode& lhs, const ErrorCode& rhs);

void FillInfoError(StringResponse& resp, ErrorCode code, std::optional<std::string_view> custom_body = std::nullopt);

}  // namespace http_handler
