#include "api.h"

#include <boost/json.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "error_codes.h"
#include "headers.h"
namespace {
using namespace std::literals;
using ec = http_handler::ErrorCodes;
}  // namespace

namespace api {

namespace ph = std::placeholders;

void ApiCommon::GetFormatData(Args_t&& args, StringResponse& resp) const {
    if (args.empty())  // Version Category
        throw ec::BAD_REQUEST;

    auto category = args.front();
    args.pop_front();

    auto it = get_handlers_.find(category);
    if (it == get_handlers_.end()) throw ec::BAD_REQUEST;
    return it->second(std::move(args), resp);
}

void ApiCommon::PostFormatData(Args_t&& args, StringResponse& resp) {
    if (args.empty())  // Version Category
        throw ec::BAD_REQUEST;

    auto category = args.front();
    args.pop_front();

    auto it = post_handlers_.find(category);
    if (it == post_handlers_.end()) throw ec::BAD_REQUEST;
    return it->second(std::move(args), resp);
}

std::string ApiCommon::GetContentTypeString(api::ApiCommon::TypeData type) {
    std::string content;
    switch (type) {
        case api::ApiCommon::TypeData::HTML:
            content = ContentType::TEXT_HTML;
        case api::ApiCommon::TypeData::JSON:
            content = ContentType::JSON;
    }
    return content;
}

}  // namespace api
