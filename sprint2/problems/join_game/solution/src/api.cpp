#include "api.h"

#include <boost/json.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "error_codes.h"
#include "headers.h"
namespace {
using namespace std::literals;
using ec = http_handler::ErrorCode;
}  // namespace

namespace api {

namespace ph = std::placeholders;

void ApiCommon::GetFormatData(HttpResource&& res) const {
    if (res.args.empty())  // Version Category
        throw ec::BAD_REQUEST;

    auto category = res.args.front();
    res.args.pop_front();

    auto it = get_handlers_.find(category);
    if (it == get_handlers_.end()) throw ec::GET_NOT_ALLOWED;
    it->second(std::move(res));
}

void ApiCommon::PostFormatData(HttpResource&& res) {
    if (res.args.empty())  // Version Category
        throw ec::BAD_REQUEST;

    auto category = res.args.front();
    res.args.pop_front();

    auto it = post_handlers_.find(category);
    if (it == post_handlers_.end()) throw ec::POST_NOT_ALLOWED;
    it->second(std::move(res));
}

}  // namespace api
