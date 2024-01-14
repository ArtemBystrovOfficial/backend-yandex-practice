#include "api.h"

#include <boost/json.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "error_codes.h"

using namespace std::literals;
namespace json = boost::json;
using boost::property_tree::ptree;

using ec = http_handler::ErrorCodes;

std::shared_ptr<api::API> api::API::GetAPI(std::string_view version_code) {
    if (version_code.size() != 2) throw ec::BAD_REQUEST;
    if (version_code.front() != 'v' || version_code.back() < '0' ||
        version_code.back() > '9')  // v1
        throw ec::BAD_REQUEST;

    switch (version_code.back() - '0') {
        case 1:
            static std::shared_ptr<API> api(dynamic_cast<API *>(new API_V1));
            return api;
            break;
        default:
            throw ec::BAD_REQUEST;
    }
}

std::string api::API_V1::getMapListJson(const model::Game &game) const {
    return game.GetJsonMaps();
}

std::string api::API_V1::getMapDescriptionJson(const model::Game &game,
                                               const std::string &id) const {
    auto *map = game.FindMap(model::Map::Id(id));
    if (!map) throw ec::MAP_NOT_FOUNDED;

    return map->GetJson();
}
