#pragma once

////////////////////////////
//// Централизация ошибок
////////////////////////////

namespace http_handler {

using error_code_t = char;

enum ErrorCodes {
    OK = 0x0,
    MAP_NOT_FOUNDED = 0x1,
    BAD_REQUEST = 0x2,
    BAD_ACCESS = 0x3,
    READ_FILE = 0x4,
    UNKNOWN_ERROR = 0x5,
    FILE_NOT_EXIST = 0x6,
    JOIN_PLAYER = 0x7
};

}  // namespace http_handler
