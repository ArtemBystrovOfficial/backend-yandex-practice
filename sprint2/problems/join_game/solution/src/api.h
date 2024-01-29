#pragma once

#include <map>
#include <memory>

#include "headers.h"
#include "model.h"
namespace api {

using version_code_t = size_t;
using Args_t = std::deque<std::string_view>;
using api_handler_t = std::function<void(Args_t&&, StringResponse& resp)>;

class ApiCommon {
   public:
    enum class TypeData { JSON, HTML, PLAIN };

    ApiCommon() = default;

    void GetFormatData(Args_t&&, StringResponse& resp) const;
    void PostFormatData(Args_t&&, StringResponse& resp);

    virtual int GetVersionCode() = 0;

    static std::string GetContentTypeString(api::ApiCommon::TypeData type);

   protected:
    std::map<std::string_view, api_handler_t> get_handlers_;
    std::map<std::string_view, api_handler_t> post_handlers_;
};

}  // namespace api