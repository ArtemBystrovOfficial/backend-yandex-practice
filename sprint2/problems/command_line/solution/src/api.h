#pragma once

#include <boost/asio.hpp>
#include <map>
#include <memory>

#include "headers.h"
#include "model.h"

namespace api {

namespace net = boost::asio;
using version_code_t = size_t;
using Args_t = std::deque<std::string_view>;
using api_handler_t = std::function<void(HttpResource&&)>;

class ApiCommon {
   public:
    enum class TypeData { JSON, HTML, PLAIN };

    explicit ApiCommon(strand_t & strand) : strand_(strand){};

    virtual void HandleApi(HttpResource&& resp) = 0;

    virtual int GetVersionCode() = 0;
    static std::string GetContentTypeString(api::ApiCommon::TypeData type);

   protected:
    strand_t & strand_;
};

}  // namespace api