#pragma once

#include <string_view>

#include "headers.h"

namespace http_handler {

class BasicRedirection {
    virtual void redirectTargetReadableAccess(std::string_view target, message_pack_t& resp) = 0;
    virtual void redirectTargetWritableAccess(std::string_view target, message_pack_t& resp) = 0;
}

}  // namespace http_handler