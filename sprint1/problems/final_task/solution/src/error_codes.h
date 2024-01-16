#pragma once

namespace http_handler {

using error_code_t = char;

enum ErrorCodes { OK = 0x0, MAP_NOT_FOUNDED = 0x1, BAD_REQUEST = 0x2 };

}  // namespace http_handler
