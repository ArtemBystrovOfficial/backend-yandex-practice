#include "api.h"

#include <boost/json.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "error_codes.h"
#include "headers.h"

using namespace std::literals;
namespace json = boost::json;
using boost::property_tree::ptree;

using ec = http_handler::ErrorCodes;

namespace api {

namespace ph = std::placeholders;

std::pair<std::string, ApiCommon::TypeData> ApiCommon::GetFormatData(Args_t&& args, std::string_view body) const {
    if (args.empty())  // Version Category
        throw ec::BAD_REQUEST;

    auto category = args.front();
    args.pop_front();

    auto it = get_handlers_.find(category);
    if (it == get_handlers_.end()) throw ec::BAD_REQUEST;
    return it->second(std::move(args), body);
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

ApiV1::ApiV1(model::Game& game) : game_(game) { get_handlers_.insert({"maps"sv, std::bind(&ApiV1::MapHandler, this, ph::_1, ph::_2)}); }

int ApiV1::GetVersionCode() { return 0x1; }

std::pair<std::string, ApiCommon::TypeData> ApiV1::MapHandler(Args_t&& args, std::string_view body) const {
    // resp.set(http::field::content_type,
    // ContentType::JSON);
    if (args.empty())
        return {GetMapListJson(), TypeData::JSON};
    else if (args.size() == 1)
        return {GetMapDescriptionJson(args.back()), TypeData::JSON};

    throw ec::BAD_REQUEST;
}

std::string api::ApiV1::GetMapListJson() const { return game_.GetJsonMaps(); }

std::string api::ApiV1::GetMapDescriptionJson(std::string_view id) const {
    auto* map = game_.FindMap(model::Map::Id(std::string(id.data(), id.size())));
    if (!map) throw ec::MAP_NOT_FOUNDED;

    return map->GetJson();
}

ApiProxyKeeper::ApiProxyKeeper(model::Game& game) {
    // V1
    auto v1 = std::make_shared<ApiV1>(game);
    apis_.insert({v1->GetVersionCode(), std::move(v1)});
}

std::shared_ptr<ApiCommon> ApiProxyKeeper::GetApiByVersion(std::string_view version_code) {
    if (version_code.size() != 2) throw ec::BAD_REQUEST;
    if (version_code.front() != 'v' || version_code.back() < '0' || version_code.back() > '9')  // v1 пока диапазон допускает
                                                                                                // лишь 1-9
        throw ec::BAD_REQUEST;

    auto api_impl = getApiByCode(version_code.back() - '0');
    if (!api_impl) throw ec::BAD_REQUEST;
    return api_impl;
}

std::shared_ptr<ApiCommon> ApiProxyKeeper::getApiByCode(version_code_t code) noexcept {
    auto it = apis_.find(code);
    return (it == apis_.end() ? nullptr : it->second);
}

}  // namespace api
